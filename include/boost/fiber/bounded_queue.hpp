
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_FIBERS_BOUNDED_QUEUE_H
#define BOOST_FIBERS_BOUNDED_QUEUE_H

#include <algorithm> // std::move()
#include <chrono>
#include <cstddef>
#include <mutex> // std::unique_lock
#include <system_error> // std::errc
#include <utility> // std::forward()

#include <boost/config.hpp>
#include <boost/intrusive_ptr.hpp>

#include <boost/fiber/exceptions.hpp>
#include <boost/fiber/condition.hpp>
#include <boost/fiber/mutex.hpp>
#include <boost/fiber/queue_op_status.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace detail {

template< typename T >
struct bounded_queue_node {
    typedef intrusive_ptr< bounded_queue_node >   ptr;

    std::size_t use_count;
    T           va;
    ptr         nxt;

    explicit bounded_queue_node( T && t) :
        use_count( 0),
        va( std::forward< T >( t) ),
        nxt() {
    }
};

template< typename T >
void intrusive_ptr_add_ref( bounded_queue_node< T > * p) {
    ++p->use_count;
}

template< typename T >
void intrusive_ptr_release( bounded_queue_node< T > * p) {
    if ( 0 == --p->use_count) {
        delete p;
    }
}

}

template< typename T >
class bounded_queue {
public:
    typedef T   value_type;

private:
    typedef detail::bounded_queue_node< value_type >      node_type;  

    enum class queue_status {
        open = 0,
        closed
    };

    queue_status                state_;
    std::size_t                 count_;
    typename node_type::ptr     head_;
    typename node_type::ptr  *  tail_;
    mutable mutex               mtx_;
    condition                   not_empty_cond_;
    condition                   not_full_cond_;
    std::size_t                 hwm_;
    std::size_t                 lwm_;

    bool is_closed_() const noexcept {
        return queue_status::closed == state_;
    }

    void close_() {
        state_ = queue_status::closed;
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

    queue_op_status push_( typename node_type::ptr const& new_node,
                           std::unique_lock< boost::fibers::mutex >& lk ) {
        if ( is_closed_() ) {
            return queue_op_status::closed;
        }

        while ( is_full_() ) {
            not_full_cond_.wait( lk);
        }

        return push_and_notify_( new_node);
    }

    queue_op_status try_push_( typename node_type::ptr const& new_node) {
        if ( is_closed_() ) {
            return queue_op_status::closed;
        }

        if ( is_full_() ) {
            return queue_op_status::full;
        }

        return push_and_notify_( new_node);
    }

    template< typename Clock, typename Duration >
    queue_op_status push_wait_until_( typename node_type::ptr const& new_node,
                                      std::chrono::time_point< Clock, Duration > const& timeout_time,
                                      std::unique_lock< boost::fibers::mutex >& lk) {
        if ( is_closed_() ) {
            return queue_op_status::closed;
        }

        while ( is_full_() ) {
            if ( cv_status::timeout == not_full_cond_.wait_until( lk, timeout_time) ) {
                return queue_op_status::timeout;
            }
        }

        return push_and_notify_( new_node);
    }

    queue_op_status push_and_notify_( typename node_type::ptr const& new_node) {
        try {
            push_tail_( new_node);
            not_empty_cond_.notify_one();

            return queue_op_status::success;
        } catch (...) {
            close_();
            throw;
        }
    }

    void push_tail_( typename node_type::ptr new_node) {
        * tail_ = new_node;
        tail_ = & new_node->nxt;
        ++count_;
    }

    value_type value_pop_() {
        BOOST_ASSERT( ! is_empty_() );

        try {
            typename node_type::ptr old_head = pop_head_();
            if ( size_() <= lwm_) {
                if ( lwm_ == hwm_) {
                    not_full_cond_.notify_one();
                } else {
                    // more than one producer could be waiting
                    // to push a value
                    not_full_cond_.notify_all();
                }
            }
            return std::move( old_head->va);
        } catch (...) {
            close_();
            throw;
        }
    }

    typename node_type::ptr pop_head_() {
        typename node_type::ptr old_head = head_;
        head_ = old_head->nxt;
        if ( ! head_) {
            tail_ = & head_;
        }
        old_head->nxt.reset();
        --count_;
        return old_head;
    }

public:
    bounded_queue( std::size_t hwm,
                   std::size_t lwm) :
        state_( queue_status::open),
        count_( 0),
        head_(),
        tail_( & head_),
        mtx_(),
        not_empty_cond_(),
        not_full_cond_(),
        hwm_( hwm),
        lwm_( lwm) {
        if ( hwm_ < lwm_) {
            throw invalid_argument( static_cast< int >( std::errc::invalid_argument),
                                    "boost fiber: high-watermark is less than low-watermark for bounded_queue");
        }
    }

    bounded_queue( std::size_t wm) :
        state_( queue_status::open),
        count_( 0),
        head_(),
        tail_( & head_),
        mtx_(),
        not_empty_cond_(),
        not_full_cond_(),
        hwm_( wm),
        lwm_( wm) {
    }

    bounded_queue( bounded_queue const&) = delete;
    bounded_queue & operator=( bounded_queue const&) = delete;

    std::size_t upper_bound() const {
        return hwm_;
    }

    std::size_t lower_bound() const {
        return lwm_;
    }

    void close() {
        std::unique_lock< mutex > lk( mtx_);
        close_();
    }

    bool is_closed() const {
        std::unique_lock< mutex > lk( mtx_);
        return is_closed_();
    }

    bool is_empty() const {
        std::unique_lock< mutex > lk( mtx_);
        return is_empty_();
    }

    bool is_full() const {
        std::unique_lock< mutex > lk( mtx_);
        return is_full_();
    }

    queue_op_status push( value_type && va)
    {
        typename node_type::ptr new_node( new node_type( std::forward< value_type >( va) ) );
        std::unique_lock< mutex > lk( mtx_);
        return push_( new_node, lk);
    }

    template< typename Rep, typename Period >
    queue_op_status push_wait_for( value_type && va,
                                   std::chrono::duration< Rep, Period > const& timeout_duration) {
        return push_wait_until( std::forward< value_type >( va),
                                std::chrono::high_resolution_clock::now() + timeout_duration);
    }

    template< typename Clock, typename Duration >
    queue_op_status push_wait_until( value_type && va,
                                     std::chrono::time_point< Clock, Duration > const& timeout_time) {
        typename node_type::ptr new_node( new node_type( std::forward< value_type >( va) ) );
        std::unique_lock< mutex > lk( mtx_);
        return push_wait_until_( new_node, timeout_time, lk);
    }

    queue_op_status try_push( value_type && va) {
        typename node_type::ptr new_node( new node_type( std::forward< value_type >( va) ) );
        std::unique_lock< mutex > lk( mtx_);
        return try_push_( new_node);
    }

    queue_op_status pop( value_type & va) {
        std::unique_lock< mutex > lk( mtx_);

        while ( ! is_closed_() && is_empty_() ) {
            not_empty_cond_.wait( lk);
        }

        if ( is_closed_() && is_empty_() ) {
            return queue_op_status::closed;
        }

        std::swap( va, value_pop_() );
        return queue_op_status::success;
    }

    value_type value_pop() {
        std::unique_lock< mutex > lk( mtx_);

        while ( ! is_closed_() && is_empty_() ) {
            not_empty_cond_.wait( lk);
        }

        if ( is_closed_() && is_empty_() ) {
            throw logic_error("boost fiber: queue is closed");
        }

        return value_pop_();
    }

    queue_op_status try_pop( value_type & va) {
        std::unique_lock< mutex > lk( mtx_);

        if ( is_closed_() && is_empty_() ) {
            return queue_op_status::closed;
        }

        if ( is_empty_() ) {
            return queue_op_status::empty;
        }

        std::swap( va, value_pop_() );
        return queue_op_status::success;
    }

    template< typename Rep, typename Period >
    queue_op_status pop_wait_for( value_type & va,
                                  std::chrono::duration< Rep, Period > const& timeout_duration) {
        return pop_wait_until( va,
                               std::chrono::high_resolution_clock::now() + timeout_duration);
    }

    template< typename Clock, typename Duration >
    queue_op_status pop_wait_until( value_type & va,
                                    std::chrono::time_point< Clock, Duration > const& timeout_time) {
        std::unique_lock< mutex > lk( mtx_);

        while ( ! is_closed_() && is_empty_() ) {
            if ( cv_status::timeout == not_empty_cond_.wait_until( lk, timeout_time) ) {
                return queue_op_status::timeout;
            }
        }

        if ( is_closed_() && is_empty_() ) {
            return queue_op_status::closed;
        }

        std::swap( va, value_pop_() );
        return queue_op_status::success;
    }
};

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_BOUNDED_QUEUE_H
