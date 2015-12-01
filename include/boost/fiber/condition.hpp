
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_CONDITION_H
#define BOOST_FIBERS_CONDITION_H

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

class BOOST_FIBERS_DECL condition {
private:
    typedef context::wait_queue_t   wait_queue_t;

    wait_queue_t        wait_queue_{};
    detail::spinlock    wait_queue_splk_{};

public:
    condition() = default;

    ~condition() noexcept {
        BOOST_ASSERT( wait_queue_.empty() );
    }

    condition( condition const&) = delete;
    condition & operator=( condition const&) = delete;

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
        typename LockType::mutex_type * mtx = lt.mutex();
        if ( ctx != mtx->owner_) {
            throw lock_error( static_cast< int >( std::errc::operation_not_permitted),
                    "boost fiber: no  privilege to perform the operation");
        }
        // store this fiber in waiting-queue
        // in order notify (resume) this fiber later
        detail::spinlock_lock lk( wait_queue_splk_);
        BOOST_ASSERT( ! ctx->wait_is_linked() );
        ctx->wait_link( wait_queue_);
        // unlock external
        lt.unlock();
        std::function< void() > func([&lk](){
                lk.unlock();
                });
        // suspend this fiber
        ctx->suspend( & func);
        try {
            // check if context was interrupted
            this_fiber::interruption_point();
        } catch ( ... ) {
            ctx->wait_unlink();
            throw;
        }
        BOOST_ASSERT( ! ctx->wait_is_linked() );
        // lock external again before returning
        lt.lock();
    }

    template< typename LockType, typename Clock, typename Duration >
    cv_status wait_until( LockType & lt, std::chrono::time_point< Clock, Duration > const& timeout_time_) {
        // check if context was interrupted
        this_fiber::interruption_point();
        cv_status status = cv_status::no_timeout;
        std::chrono::steady_clock::time_point timeout_time(
                detail::convert( timeout_time_) );
        context * ctx = context::active();
        typename LockType::mutex_type * mtx = lt.mutex();
        if ( ctx != mtx->owner_) {
            throw lock_error( static_cast< int >( std::errc::operation_not_permitted),
                    "boost fiber: no  privilege to perform the operation");
        }
        // store this fiber in waiting-queue
        // in order notify (resume) this fiber later
        detail::spinlock_lock lk( wait_queue_splk_);
        BOOST_ASSERT( ! ctx->wait_is_linked() );
        ctx->wait_link( wait_queue_);
        // unlock external
        lt.unlock();
        std::function< void() > func([&lk](){
                lk.unlock();
                });
        // suspend this fiber
        if ( ! ctx->wait_until( timeout_time, & func) ) {
            status = cv_status::timeout;
            ctx->wait_unlink();
        }
        try {
            // check if context was interrupted
            this_fiber::interruption_point();
        } catch ( ... ) {
            ctx->wait_unlink();
            throw;
        }
        BOOST_ASSERT( ! ctx->wait_is_linked() );
        // lock external again before returning
        lt.lock();
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

typedef condition condition_variable;
typedef condition condition_variable_any;

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_CONDITION_H
