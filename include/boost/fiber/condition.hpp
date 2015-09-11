
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_CONDITION_H
#define BOOST_FIBERS_CONDITION_H

#include <algorithm>
#include <chrono>
#include <mutex>

#include <boost/assert.hpp>
#include <boost/config.hpp>

#include <boost/fiber/context.hpp>
#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/clock_cast.hpp>
#include <boost/fiber/detail/queues.hpp>
#include <boost/fiber/detail/spinlock.hpp>
#include <boost/fiber/interruption.hpp>
#include <boost/fiber/mutex.hpp>
#include <boost/fiber/operations.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

class context;

enum class cv_status {
    no_timeout = 1,
    timeout
};

class BOOST_FIBERS_DECL condition {
private:
    typedef detail::wait_queue< context >   wait_queue_t;

    detail::spinlock    splk_;
    wait_queue_t        wait_queue_;

public:
    condition();

    ~condition();

    condition( condition const&) = delete;
    condition & operator=( condition const&) = delete;

    void notify_one();

    void notify_all();

    template< typename LockType, typename Pred >
    void wait( LockType & lt, Pred pred) {
        while ( ! pred() ) {
            wait( lt);
        }
    }

    template< typename LockType >
    void wait( LockType & lt) {
        context * f( context::active() );
        try {
            // lock spinlock
            detail::spinlock_lock lk( splk_);

            // store this fiber in waiting-queue
            // in order notify (resume) this fiber later
            BOOST_ASSERT( ! f->wait_is_linked() );
            wait_queue_.push_back( * f);
            lk.unlock();

            // unlock external
            lt.unlock();

            // check if fiber was interrupted
            this_fiber::interruption_point();
            // suspend this fiber
            f->do_schedule();

            // lock external again before returning
            lt.lock();
        } catch (...) {
            detail::spinlock_lock lk( splk_);
            f->wait_unlink();
            throw;
        }
    }

    template< typename LockType, typename Clock, typename Duration >
    cv_status wait_until( LockType & lt, std::chrono::time_point< Clock, Duration > const& timeout_time_) {
        cv_status status = cv_status::no_timeout;
        std::chrono::steady_clock::time_point timeout_time(
                detail::clock_cast( timeout_time_) );
        context * f( context::active() );
        try {
            // lock spinlock
            detail::spinlock_lock lk( splk_);

            // store this fiber in waiting-queue
            // in order notify (resume) this fiber later
            BOOST_ASSERT( ! f->wait_is_linked() );
            wait_queue_.push_back( * f);
            lk.unlock();

            // unlock external
            lt.unlock();

            // check if fiber was interrupted
            this_fiber::interruption_point();
            // suspend this fiber
            if ( ! f->do_wait_until( timeout_time) ) {
                // this fiber was not notified before timeout
                // lock spinlock again
                detail::spinlock_lock lk( splk_);
                f->wait_unlink();

                status = cv_status::timeout;
            }

            // lock external again before returning
            lt.lock();
        } catch (...) {
            detail::spinlock_lock lk( splk_);
            f->wait_unlink();
            throw;
        }

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
