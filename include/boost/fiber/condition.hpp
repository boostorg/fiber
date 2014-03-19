
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_CONDITION_H
#define BOOST_FIBERS_CONDITION_H

#include <algorithm>
#include <cstddef>
#include <deque>

#include <boost/assert.hpp>
#include <boost/chrono/system_clocks.hpp>
#include <boost/config.hpp>
#include <boost/detail/scoped_enum_emulation.hpp>
#include <boost/thread/locks.hpp>
#include <boost/utility.hpp>

#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/main_fiber.hpp>
#include <boost/fiber/detail/fiber_base.hpp>
#include <boost/fiber/detail/scheduler.hpp>
#include <boost/fiber/detail/spinlock.hpp>
#include <boost/fiber/exceptions.hpp>
#include <boost/fiber/interruption.hpp>
#include <boost/fiber/mutex.hpp>
#include <boost/fiber/operations.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

# if defined(BOOST_MSVC)
# pragma warning(push)
# pragma warning(disable:4355 4251 4275)
# endif

namespace boost {
namespace fibers {

BOOST_SCOPED_ENUM_DECLARE_BEGIN(cv_status)
{
    no_timeout = 1,
    timeout
}
BOOST_SCOPED_ENUM_DECLARE_END(cv_status)

class BOOST_FIBERS_DECL condition : private noncopyable
{
private:
    detail::spinlock                        splk_;
    std::deque< detail::fiber_base * >      waiting_;

public:
    condition();

    ~condition();

    void notify_one();

    void notify_all();

    template< typename LockType, typename Pred >
    void wait( LockType & lt, Pred pred)
    {
        while ( ! pred() )
            wait( lt);
    }

    template< typename LockType >
    void wait( LockType & lt)
    {
        detail::fiber_base * n( detail::scheduler::instance()->active() );
        try
        {
            if ( n)
            {
                // lock spinlock
                unique_lock< detail::spinlock > lk( splk_);

                BOOST_ASSERT( waiting_.end() == std::find( waiting_.begin(), waiting_.end(), n) );
                // store this fiber in waiting-queue
                // in order notify (resume) this fiber later
                waiting_.push_back( n);

                // unlock external
                lt.unlock();

                // suspend this fiber
                // locked spinlock will be released if this fiber
                // was stored inside schedulers's waiting-queue
                detail::scheduler::instance()->wait( lk);

                // this fiber was notified and resumed
                // check if fiber was interrupted
                this_fiber::interruption_point();

                // lock external again before returning
                lt.lock();
            }
            else
            {
                // notifier for main-fiber
                n = detail::scheduler::instance()->get_main_fiber();

                // lock spinlock
                unique_lock< detail::spinlock > lk( splk_);

                BOOST_ASSERT( waiting_.end() == std::find( waiting_.begin(), waiting_.end(), n) );
                // store this main-notifier in waiting-queue
                // in order to be notified later
                waiting_.push_back( n);

                // unlock external
                lt.unlock();

                // release spinlock
                lk.unlock();

                // loop until main-notifier gets notified
                while ( ! n->is_ready() )
                    // run scheduler
                    detail::scheduler::instance()->run();

                // lock external again before returning
                lt.lock();
            }
        }
        catch (...)
        {
            unique_lock< detail::spinlock > lk( splk_);
            // remove fiber from waiting-list
            waiting_.erase(
                    std::find( waiting_.begin(), waiting_.end(), n) );
            throw;
        }
    }

    template< typename LockType >
    cv_status wait_until( LockType & lt, clock_type::time_point const& timeout_time)
    {
        cv_status status = cv_status::no_timeout;

        detail::fiber_base * n( detail::scheduler::instance()->active() );
        try
        {
            if ( n)
            {
                // lock spinlock
                unique_lock< detail::spinlock > lk( splk_);

                // store this fiber in waiting-queue
                // in order notify (resume) this fiber later
                waiting_.push_back( n);

                // unlock external
                lt.unlock();

                // suspend this fiber
                // locked spinlock will be released if this fiber
                // was stored inside schedulers's waiting-queue
                if ( ! detail::scheduler::instance()->wait_until( timeout_time, lk) )
                {
                    // this fiber was not notified before timeout
                    // lock spinlock again
                    unique_lock< detail::spinlock > lk( splk_);
                    // remove fiber from waiting-list
                    waiting_.erase(
                        std::find( waiting_.begin(), waiting_.end(), n) );

                    status = cv_status::timeout;
                }

                // check if fiber was interrupted
                this_fiber::interruption_point();

                // lock external again before returning
                lt.lock();
            }
            else
            {
                // notifier for main-fiber
                n = detail::scheduler::instance()->get_main_fiber();

                // lock spinlock
                unique_lock< detail::spinlock > lk( splk_);

                // store this fiber in order to be notified later
                waiting_.push_back( n);

                // unlock external
                lt.unlock();

                // release spinlock
                lk.unlock();

                // loop until main-notifier gets notified
                while ( ! n->is_ready() )
                {
                    // check timepoint
                    if ( ! ( clock_type::now() < timeout_time) )
                    {
                        // timeout happend before notified
                        // lock spinlock
                        unique_lock< detail::spinlock > lk( splk_);
                        // remove fiber from waiting-list
                        waiting_.erase(
                                std::find( waiting_.begin(), waiting_.end(), n) );

                        status = cv_status::timeout;

                        break;
                    }
                    // run scheduler
                    detail::scheduler::instance()->run();
                }

                // lock external again before returning
                lt.lock();
            }
        }
        catch (...)
        {
            unique_lock< detail::spinlock > lk( splk_);
            // remove fiber from waiting-list
            waiting_.erase(
                std::find( waiting_.begin(), waiting_.end(), n) );
            throw;
        }

        return status;
    }

    template< typename LockType, typename Pred >
    bool wait_until( LockType & lt, clock_type::time_point const& timeout_time, Pred pred)
    {
        while ( ! pred() )
        {
            if ( cv_status::timeout == wait_until( lt, timeout_time) )
                return pred();
        }
        return true;
    }

    template< typename LockType, typename Rep, typename Period >
    cv_status wait_for( LockType & lt, chrono::duration< Rep, Period > const& timeout_duration)
    { return wait_until( lt, clock_type::now() + timeout_duration); }

    template< typename LockType, typename Rep, typename Period, typename Pred >
    bool wait_for( LockType & lt, chrono::duration< Rep, Period > const& timeout_duration, Pred pred)
    {
        while ( ! pred() )
        {
            if ( cv_status::timeout == wait_for( lt, timeout_duration) )
                return pred();
        }
        return true;
    }
};

typedef condition condition_variable;
typedef condition condition_variable_any;

}}

# if defined(BOOST_MSVC)
# pragma warning(pop)
# endif

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_CONDITION_H
