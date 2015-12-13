
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_CONDITION_VARIABLE_H
#define BOOST_FIBERS_CONDITION_VARIABLE_H

#include <algorithm>
#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>

#include <boost/assert.hpp>
#include <boost/config.hpp>

#include <boost/fiber/context.hpp>
#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/convert.hpp>
#include <boost/fiber/detail/spinlock.hpp>
#include <boost/fiber/exceptions.hpp>
#include <boost/fiber/interruption.hpp>
#include <boost/fiber/operations.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

enum class cv_status {
    no_timeout = 1,
    timeout
};

class BOOST_FIBERS_DECL condition_variable {
private:
    typedef context::wait_queue_t   wait_queue_t;

    wait_queue_t        wait_queue_{};
    detail::spinlock    wait_queue_splk_{};

public:
    condition_variable() = default;

    ~condition_variable() {
        BOOST_ASSERT( wait_queue_.empty() );
    }

    condition_variable( condition_variable const&) = delete;
    condition_variable & operator=( condition_variable const&) = delete;

    void notify_one() noexcept;

    void notify_all() noexcept;

    template< typename LockType, typename Pred >
    void wait( LockType & lt, Pred pred) {
        while ( ! pred() ) {
            wait( lt);
        }
    }

    template< typename LockType >
    void wait( LockType & lt) {
        // check if context was interrupted
        this_fiber::interruption_point();
        context * ctx = context::active();
        // pre-conditions
        BOOST_ASSERT( lt.owns_lock() && ctx == lt.mutex()->owner_);
        // atomically call lt.unlock() and block on *this
        // store this fiber in waiting-queue
        detail::spinlock_lock lk( wait_queue_splk_);
        BOOST_ASSERT( ! ctx->wait_is_linked() );
        ctx->wait_link( wait_queue_);
        // unlock external lt
        lt.unlock();
        // suspend this fiber
        ctx->suspend( lk);
        // relock local lk
        lk.lock();
        // remove from waiting-queue
        ctx->wait_unlink();
        // unlock local lk
        lk.unlock();
        // relock external again before returning
        try {
            lt.lock();
        } catch (...) {
            std::terminate();
        }
        // post-conditions
        BOOST_ASSERT( lt.owns_lock() && ctx == lt.mutex()->owner_);
        BOOST_ASSERT( ! ctx->wait_is_linked() );
        // check if context was interrupted
        this_fiber::interruption_point();
    }

    template< typename LockType, typename Clock, typename Duration >
    cv_status wait_until( LockType & lt, std::chrono::time_point< Clock, Duration > const& timeout_time_) {
        // check if context was interrupted
        this_fiber::interruption_point();
        cv_status status = cv_status::no_timeout;
        std::chrono::steady_clock::time_point timeout_time(
                detail::convert( timeout_time_) );
        context * ctx = context::active();
        // pre-conditions
        BOOST_ASSERT( lt.owns_lock() && ctx == lt.mutex()->owner_);
        // atomically call lt.unlock() and block on *this
        // store this fiber in waiting-queue
        detail::spinlock_lock lk( wait_queue_splk_);
        BOOST_ASSERT( ! ctx->wait_is_linked() );
        ctx->wait_link( wait_queue_);
        // unlock external lt
        lt.unlock();
        // suspend this fiber
        if ( ! ctx->wait_until( timeout_time, lk) ) {
            status = cv_status::timeout;
        }
        // relock local lk
        lk.lock();
        // remove from waiting-queue
        ctx->wait_unlink();
        // unlock local lk
        lk.unlock();
        // relock external again before returning
        try {
            lt.lock();
        } catch (...) {
            std::terminate();
        }
        // post-conditions
        BOOST_ASSERT( lt.owns_lock() && ctx == lt.mutex()->owner_);
        BOOST_ASSERT( ! ctx->wait_is_linked() );
        // check if context was interrupted
        this_fiber::interruption_point();
        return status;
    }

    template< typename LockType, typename Clock, typename Duration, typename Pred >
    bool wait_until( LockType & lt,
                     std::chrono::time_point< Clock, Duration > const& timeout_time, Pred pred) {
        while ( ! pred() ) {
            if ( cv_status::timeout == wait_until( lt, timeout_time) ) {
                return pred();
            }
        }
        return true;
    }

    template< typename LockType, typename Rep, typename Period >
    cv_status wait_for( LockType & lt, std::chrono::duration< Rep, Period > const& timeout_duration) {
        return wait_until( lt,
                           std::chrono::steady_clock::now() + timeout_duration);
    }

    template< typename LockType, typename Rep, typename Period, typename Pred >
    bool wait_for( LockType & lt, std::chrono::duration< Rep, Period > const& timeout_duration, Pred pred) {
        return wait_until( lt,
                           std::chrono::steady_clock::now() + timeout_duration,
                           pred);
    }
};

using condition_variable_any = condition_variable;

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_CONDITION_VARIABLE_H
