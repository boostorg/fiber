
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_FIBERS_UNBOUNDED_CHANNEL_H
#define BOOST_FIBERS_UNBOUNDED_CHANNEL_H

#include <algorithm> // std::move()
#include <chrono>
#include <cstddef>
#include <memory> // std::allocator
#include <mutex> // std::unique_lock
#include <utility> // std::forward()

#include <boost/config.hpp>
#include <boost/intrusive_ptr.hpp>

#include <boost/fiber/channel_op_status.hpp>
#include <boost/fiber/condition.hpp>
#include <boost/fiber/exceptions.hpp>
#include <boost/fiber/mutex.hpp>
#include <boost/fiber/operations.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

template< typename T, typename Allocator = std::allocator< T > >
class unbounded_channel {
public:
    typedef T   value_type;

private:
    struct node {
        typedef intrusive_ptr< node >                                     ptr;
        typedef typename std::allocator_traits< Allocator >::template rebind_alloc< node >  allocator_type;

        std::size_t         use_count{ 0 };
        allocator_type  &   alloc;
        T                   va;
        ptr                 nxt{};

        node( T const& t, allocator_type & alloc_) noexcept :
            alloc{ alloc_ },
            va{ t } {
        }

        node( T && t, allocator_type & alloc_) noexcept :
            alloc{ alloc_ },
            va{ std::forward< T >( t) } {
        }

        friend
        void intrusive_ptr_add_ref( node * p) noexcept {
            ++p->use_count;
        }

        friend
        void intrusive_ptr_release( node * p) noexcept {
            if ( 0 == --p->use_count) {
                allocator_type & alloc( p->alloc);
                std::allocator_traits< allocator_type >::destroy( alloc, p);
                std::allocator_traits< allocator_type >::deallocate( alloc, p, 1);
            }
        }
    };

    typedef typename std::allocator_traits< Allocator >::template rebind_alloc< node >   allocator_type;

    enum class queue_status {
        open = 0,
        closed
    };

    allocator_type         alloc_;
    queue_status           state_{ queue_status::open };
    typename node::ptr     head_{};
    typename node::ptr  *  tail_;
    mutable mutex          mtx_{};
    condition              not_empty_cond_{};

    bool is_closed_() const noexcept {
        return queue_status::closed == state_;
    }

    void close_( std::unique_lock< mutex > & lk) noexcept {
        state_ = queue_status::closed;
        lk.unlock();
        not_empty_cond_.notify_all();
    }

    bool is_empty_() const noexcept {
        return ! head_;
    }

    channel_op_status push_( typename node::ptr const& new_node,
                             std::unique_lock< mutex > & lk) noexcept {
        if ( is_closed_() ) {
            return channel_op_status::closed;
        }
        return push_and_notify_( new_node, lk);
    }

    channel_op_status push_and_notify_( typename node::ptr const& new_node,
                                        std::unique_lock< mutex > & lk) noexcept {
        push_tail_( new_node);
        lk.unlock();
        not_empty_cond_.notify_one();
        return channel_op_status::success;
    }

    void push_tail_( typename node::ptr new_node) noexcept {
        * tail_ = new_node;
        tail_ = & new_node->nxt;
    }

    value_type value_pop_( std::unique_lock< mutex > & lk) noexcept {
        BOOST_ASSERT( ! is_empty_() );
        auto old_head = pop_head_();
        return std::move( old_head->va);
    }

    typename node::ptr pop_head_() noexcept {
        auto old_head = head_;
        head_ = old_head->nxt;
        if ( ! head_) {
            tail_ = & head_;
        }
        old_head->nxt.reset();
        return old_head;
    }

public:
    explicit unbounded_channel( Allocator const& alloc = Allocator() ) noexcept :
        alloc_{ alloc },
        tail_{ & head_ } {
    }

    unbounded_channel( unbounded_channel const&) = delete;
    unbounded_channel & operator=( unbounded_channel const&) = delete;

    void close() noexcept {
        std::unique_lock< mutex > lk( mtx_);
        close_( lk);
    }

    channel_op_status push( value_type const& va) noexcept {
        typename node::ptr new_node(
            new ( alloc_.allocate( 1) ) node( va, alloc_) );
        std::unique_lock< mutex > lk( mtx_);
        return push_( new_node, lk);
    }

    channel_op_status push( value_type && va) noexcept {
        typename node::ptr new_node(
            new ( alloc_.allocate( 1) ) node( std::forward< value_type >( va), alloc_) );
        std::unique_lock< mutex > lk( mtx_);
        return push_( new_node, lk);
    }

    channel_op_status pop( value_type & va) {
        std::unique_lock< mutex > lk( mtx_);
        while ( ! is_closed_() && is_empty_() ) {
            not_empty_cond_.wait( lk);
        }
        if ( is_closed_() && is_empty_() ) {
            return channel_op_status::closed;
        }
        va = value_pop_( lk);
        return channel_op_status::success;
    }

    value_type value_pop() {
        std::unique_lock< mutex > lk( mtx_);
        while ( ! is_closed_() && is_empty_() ) {
            not_empty_cond_.wait( lk);
        }
        if ( is_closed_() && is_empty_() ) {
            throw logic_error("boost fiber: queue is closed");
        }
        return value_pop_( lk);
    }

    channel_op_status try_pop( value_type & va) noexcept {
        std::unique_lock< mutex > lk( mtx_);
        if ( is_closed_() && is_empty_() ) {
            // let other fibers run
            lk.unlock();
            this_fiber::yield();
            return channel_op_status::closed;
        }
        if ( is_empty_() ) {
            // let other fibers run
            lk.unlock();
            this_fiber::yield();
            return channel_op_status::empty;
        }
        va = value_pop_( lk);
        return channel_op_status::success;
    }

    template< typename Rep, typename Period >
    channel_op_status pop_wait_for( value_type & va,
                                    std::chrono::duration< Rep, Period > const& timeout_duration) {
        return pop_wait_until( va, std::chrono::steady_clock::now() + timeout_duration);
    }

    template< typename Clock, typename Duration >
    channel_op_status pop_wait_until( value_type & va,
                                      std::chrono::time_point< Clock, Duration > const& timeout_time) {
        std::unique_lock< mutex > lk( mtx_);
        while ( ! is_closed_() && is_empty_() ) {
            if ( cv_status::timeout == not_empty_cond_.wait_until( lk, timeout_time) )
                return channel_op_status::timeout;
        }
        if ( is_closed_() && is_empty_() ) {
            return channel_op_status::closed;
        }
        va = value_pop_( lk);
        return channel_op_status::success;
    }
};

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_UNBOUNDED_CHANNEL_H
