
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_FIBERS_BOUNDED_CHANNEL_H
#define BOOST_FIBERS_BOUNDED_CHANNEL_H

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <memory>
#include <mutex>
#include <utility>

#include <boost/config.hpp>
#include <boost/intrusive_ptr.hpp>

#include <boost/fiber/exceptions.hpp>
#include <boost/fiber/condition.hpp>
#include <boost/fiber/mutex.hpp>
#include <boost/fiber/channel_op_status.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

template< typename T, typename Allocator = std::allocator< T > >
class bounded_channel {
public:
    typedef T   value_type;

private:
    struct node {
        typedef intrusive_ptr< node >                                     ptr;
        typedef typename std::allocator_traits< Allocator >::template rebind_alloc< node >  allocator_type;

        std::size_t         use_count;
        allocator_type  &   alloc;
        T                   va;
        ptr                 nxt;

        explicit node( T const& t, allocator_type & alloc_) :
            use_count( 0),
            alloc( alloc_),
            va( t),
            nxt() {
        }

        explicit node( T && t, allocator_type & alloc_) :
            use_count( 0),
            alloc( alloc_),
            va( std::forward< T >( t) ),
            nxt() {
        }

        friend
        void intrusive_ptr_add_ref( node * p) {
            ++p->use_count;
        }

        friend
        void intrusive_ptr_release( node * p) {
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
    queue_status           state_;
    std::size_t            count_;
    typename node::ptr     head_;
    typename node::ptr  *  tail_;
    mutable mutex          mtx_;
    condition              not_empty_cond_;
    condition              not_full_cond_;
    std::size_t            hwm_;
    std::size_t            lwm_;

    bool is_closed_() const noexcept {
        return queue_status::closed == state_;
    }

    void close_( std::unique_lock< boost::fibers::mutex > & lk) {
        state_ = queue_status::closed;
        lk.unlock();
        not_empty_cond_.notify_all();
        not_full_cond_.notify_all();
    }

    std::size_t size_() const noexcept {
        return count_;
    }

    bool is_empty_() const noexcept {
        return ! head_;
    }

    bool is_full_() const noexcept {
        return count_ >= hwm_;
    }

    channel_op_status push_( typename node::ptr const& new_node,
                             std::unique_lock< boost::fibers::mutex > & lk) {
        if ( is_closed_() ) {
            return channel_op_status::closed;
        }
        while ( is_full_() ) {
            not_full_cond_.wait( lk);
        }
        return push_and_notify_( new_node, lk);
    }

    channel_op_status try_push_( typename node::ptr const& new_node,
                                 std::unique_lock< boost::fibers::mutex > & lk) {
        if ( is_closed_() ) {
            return channel_op_status::closed;
        }
        if ( is_full_() ) {
            return channel_op_status::full;
        }
        return push_and_notify_( new_node, lk);
    }

    template< typename Clock, typename Duration >
    channel_op_status push_wait_until_( typename node::ptr const& new_node,
                                        std::chrono::time_point< Clock, Duration > const& timeout_time,
                                        std::unique_lock< boost::fibers::mutex > & lk) {
        if ( is_closed_() ) {
            return channel_op_status::closed;
        }
        while ( is_full_() ) {
            if ( cv_status::timeout == not_full_cond_.wait_until( lk, timeout_time) ) {
                return channel_op_status::timeout;
            }
        }
        return push_and_notify_( new_node, lk);
    }

    channel_op_status push_and_notify_( typename node::ptr const& new_node,
                                        std::unique_lock< boost::fibers::mutex > & lk) {
        try {
            push_tail_( new_node);
            lk.unlock();
            not_empty_cond_.notify_one();
            return channel_op_status::success;
        } catch (...) {
            close_( lk);
            throw;
        }
    }

    void push_tail_( typename node::ptr new_node) {
        * tail_ = new_node;
        tail_ = & new_node->nxt;
        ++count_;
    }

    value_type value_pop_( std::unique_lock< boost::fibers::mutex > & lk) {
        BOOST_ASSERT( ! is_empty_() );
        try {
            typename node::ptr old_head = pop_head_();
            if ( size_() <= lwm_) {
                if ( lwm_ == hwm_) {
                    lk.unlock();
                    not_full_cond_.notify_one();
                } else {
                    lk.unlock();
                    // more than one producer could be waiting
                    // to push a value
                    not_full_cond_.notify_all();
                }
            }
            return std::move( old_head->va);
        } catch (...) {
            close_( lk);
            throw;
        }
    }

    typename node::ptr pop_head_() {
        typename node::ptr old_head = head_;
        head_ = old_head->nxt;
        if ( ! head_) {
            tail_ = & head_;
        }
        old_head->nxt.reset();
        --count_;
        return old_head;
    }

public:
    bounded_channel( std::size_t hwm,
                   std::size_t lwm,
                   Allocator const& alloc = Allocator() ) :
        alloc_( alloc),
        state_( queue_status::open),
        count_( 0),
        head_(),
        tail_( & head_),
        mtx_(),
        not_empty_cond_(),
        not_full_cond_(),
        hwm_( hwm),
        lwm_( lwm) {
        if ( hwm_ <= lwm_) {
            throw invalid_argument( static_cast< int >( std::errc::invalid_argument),
                                    "boost fiber: high-watermark is less than or equal to low-watermark for bounded_channel");
        }
        if ( 0 == hwm) {
            throw invalid_argument( static_cast< int >( std::errc::invalid_argument),
                                    "boost fiber: high-watermark is zero");
        }
    }

    bounded_channel( std::size_t wm,
                   Allocator const& alloc = Allocator() ) :
        alloc_( alloc),
        state_( queue_status::open),
        count_( 0),
        head_(),
        tail_( & head_),
        mtx_(),
        not_empty_cond_(),
        not_full_cond_(),
        hwm_( wm),
        lwm_() {
        if ( 0 == wm) {
            throw invalid_argument( static_cast< int >( std::errc::invalid_argument),
                                    "boost fiber: watermark is zero");
        }
        lwm_ = hwm_ - 1;
    }

    bounded_channel( bounded_channel const&) = delete;
    bounded_channel & operator=( bounded_channel const&) = delete;

    std::size_t upper_bound() const {
        return hwm_;
    }

    std::size_t lower_bound() const {
        return lwm_;
    }

    void close() {
        std::unique_lock< mutex > lk( mtx_);
        close_( lk);
    }

    channel_op_status push( value_type const& va) {
        typename node::ptr new_node(
            new ( alloc_.allocate( 1) ) node( va, alloc_) );
        std::unique_lock< mutex > lk( mtx_);
        return push_( new_node, lk);
    }

    channel_op_status push( value_type && va) {
        typename node::ptr new_node(
            new ( alloc_.allocate( 1) ) node( std::forward< value_type >( va), alloc_) );
        std::unique_lock< mutex > lk( mtx_);
        return push_( new_node, lk);
    }

    template< typename Rep, typename Period >
    channel_op_status push_wait_for( value_type const& va,
                                     std::chrono::duration< Rep, Period > const& timeout_duration) {
        return push_wait_until( va,
                                std::chrono::steady_clock::now() + timeout_duration);
    }

    template< typename Rep, typename Period >
    channel_op_status push_wait_for( value_type && va,
                                     std::chrono::duration< Rep, Period > const& timeout_duration) {
        return push_wait_until( std::forward< value_type >( va),
                                std::chrono::steady_clock::now() + timeout_duration);
    }

    template< typename Clock, typename Duration >
    channel_op_status push_wait_until( value_type const& va,
                                       std::chrono::time_point< Clock, Duration > const& timeout_time) {
        typename node::ptr new_node(
            new ( alloc_.allocate( 1) ) node( va, alloc_) );
        std::unique_lock< mutex > lk( mtx_);
        return push_wait_until_( new_node, timeout_time, lk);
    }

    template< typename Clock, typename Duration >
    channel_op_status push_wait_until( value_type && va,
                                       std::chrono::time_point< Clock, Duration > const& timeout_time) {
        typename node::ptr new_node(
            new ( alloc_.allocate( 1) ) node( std::forward< value_type >( va), alloc_) );
        std::unique_lock< mutex > lk( mtx_);
        return push_wait_until_( new_node, timeout_time, lk);
    }

    channel_op_status try_push( value_type const& va) {
        typename node::ptr new_node(
            new ( alloc_.allocate( 1) ) node( va, alloc_) );
        std::unique_lock< mutex > lk( mtx_);
        return try_push_( new_node, lk);
    }

    channel_op_status try_push( value_type && va) {
        typename node::ptr new_node(
            new ( alloc_.allocate( 1) ) node( std::forward< value_type >( va), alloc_) );
        std::unique_lock< mutex > lk( mtx_);
        return try_push_( new_node, lk);
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

    channel_op_status try_pop( value_type & va) {
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
        return pop_wait_until( va,
                               std::chrono::steady_clock::now() + timeout_duration);
    }

    template< typename Clock, typename Duration >
    channel_op_status pop_wait_until( value_type & va,
                                      std::chrono::time_point< Clock, Duration > const& timeout_time) {
        std::unique_lock< mutex > lk( mtx_);
        while ( ! is_closed_() && is_empty_() ) {
            if ( cv_status::timeout == not_empty_cond_.wait_until( lk, timeout_time) ) {
                return channel_op_status::timeout;
            }
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

#endif // BOOST_FIBERS_BOUNDED_CHANNEL_H
