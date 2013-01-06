
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
//  based on boost::interprocess::sync::interprocess_condition

#ifndef BOOST_FIBERS_CONDITION_H
#define BOOST_FIBERS_CONDITION_H

#include <cstddef>
#include <deque>

#include <boost/assert.hpp>
#include <boost/atomic.hpp>
#include <boost/chrono/system_clocks.hpp>
#include <boost/config.hpp>
#include <boost/thread/locks.hpp>
#include <boost/utility.hpp>

#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/scheduler.hpp>
#include <boost/fiber/detail/fiber_base.hpp>
#include <boost/fiber/detail/spin_mutex.hpp>
#include <boost/fiber/exceptions.hpp>
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

class BOOST_FIBERS_DECL condition : private noncopyable
{
private:
    enum command
    {
        SLEEPING = 0,
        NOTIFY_ONE,
        NOTIFY_ALL
    };

    atomic< command >                       cmd_;
    atomic< std::size_t >                   waiters_;
    mutex                                   enter_mtx_;
    mutex                                   check_mtx_;
    detail::spin_mutex                      waiting_mtx_;
    std::deque< detail::fiber_base::ptr_t > waiting_;

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
        BOOST_ASSERT( this_fiber::is_fiberized() );

        {
            mutex::scoped_lock lk( enter_mtx_);
            BOOST_ASSERT( lk);
            ++waiters_;
            lt.unlock();
        }

        bool unlock_enter_mtx = false;

        //Loop until a notification indicates that the thread should exit
        for (;;)
        {
            //The thread sleeps/spins until a spin_condition commands a notification
            //Notification occurred, we will lock the checking mutex so that
            while ( SLEEPING == cmd_)
            {
                detail::spin_mutex::scoped_lock lk( waiting_mtx_);
                waiting_.push_back(
                    detail::scheduler::instance().active() );
                detail::scheduler::instance().wait( lk);
            }

			command expected = NOTIFY_ONE;
			cmd_.compare_exchange_strong( expected, SLEEPING);
			if ( SLEEPING == expected)
                //Other thread has been notified and since it was a NOTIFY one
                //command, this thread must sleep again
				continue;
			else if ( NOTIFY_ONE == expected)
			{
                //If it was a NOTIFY_ONE command, only this thread should
                //exit. This thread has atomically marked command as sleep before
                //so no other thread will exit.
                //Decrement wait count.
				unlock_enter_mtx = true;
				--waiters_;
				break;
			}
            else
            {
                //If it is a NOTIFY_ALL command, all threads should return
                //from do_timed_wait function. Decrement wait count.
				unlock_enter_mtx = 0 == --waiters_;
                //Check if this is the last thread of notify_all waiters
                //Only the last thread will release the mutex
                if ( unlock_enter_mtx)
				{
					expected = NOTIFY_ALL;
					cmd_.compare_exchange_strong( expected, SLEEPING);
				}
                break;
            }
        }

        //Unlock the enter mutex if it is a single notification, if this is
        //the last notified thread in a notify_all or a timeout has occurred
        if ( unlock_enter_mtx)
            enter_mtx_.unlock();

        //Lock external again before returning from the method
        lt.lock();
    }

    template< typename LockType, typename TimeDuration > 
    bool timed_wait( LockType & lt, TimeDuration const& dt)
    { return timed_wait( lt, chrono::system_clock::now() + dt); }

    template< typename LockType, typename TimeDuration, typename Pred > 
    bool timed_wait( LockType & lt, TimeDuration const& dt, Pred pred)
    { return timed_wait( lt, chrono::system_clock::now() + dt, pred); }

    template< typename LockType, typename Pred > 
    bool timed_wait( LockType & lt, chrono::system_clock::time_point const& abs_time, Pred pred)
    {
        while ( ! pred() )
            if ( ! timed_wait( lt, abs_time) )
                return pred();
        return true;
    }

    template< typename LockType >
    bool timed_wait( LockType & lt, chrono::system_clock::time_point const& abs_time)
    {
        BOOST_ASSERT( this_fiber::is_fiberized() );

        if ( (chrono::system_clock::time_point::max)() == abs_time){
            wait( lt);
            return true;
        }
        chrono::system_clock::time_point now( chrono::system_clock::now() );
        if ( now >= abs_time) return false;

        {
            mutex::scoped_lock lk( enter_mtx_); // FIXME: abs_time!
            BOOST_ASSERT( lk);
            ++waiters_;
            lt.unlock();
        }

        bool unlock_enter_mtx = false, timed_out = false;
        //Loop until a notification indicates that the thread should
        //exit or timeout occurs
        for (;;)
        {
            //The thread sleeps/spins until a spin_condition commands a notification
            //Notification occurred, we will lock the checking mutex so that
            while ( SLEEPING == cmd_)
            {
#if 0
                detail::spin_mutex::scoped_lock lk( waiting_mtx_);
                waiting_.push_back(
                    detail::scheduler::instance().active() );
                detail::scheduler::instance().wait( lk);
#endif
                this_fiber::yield();

                now = chrono::system_clock::now();
                if ( now >= abs_time)
                {
                    //If we can lock the mutex it means that no notification
                    //is being executed in this spin_condition variable
                    timed_out = enter_mtx_.try_lock();

                    //If locking fails, indicates that another thread is executing
                    //notification, so we play the notification game
                    if ( ! timed_out)
                        //There is an ongoing notification, we will try again later
                        continue;

                    //No notification in execution, since enter mutex is locked.
                    //We will execute time-out logic, so we will decrement count,
                    //release the enter mutex and return false.
                    break; 
                }
            }

            //If a timeout occurred, the mutex will not execute checking logic
            if ( timed_out)
            {
                unlock_enter_mtx = true;
                --waiters_;
                break;
            }
            else
            {
                command expected = NOTIFY_ONE;
                cmd_.compare_exchange_strong( expected, SLEEPING);
                if ( SLEEPING == expected)
                    //Other thread has been notified and since it was a NOTIFY one
                    //command, this thread must sleep again
                    continue;
                else if ( NOTIFY_ONE == expected)
                {
                    //If it was a NOTIFY_ONE command, only this thread should
                    //exit. This thread has atomically marked command as sleep before
                    //so no other thread will exit.
                    //Decrement wait count.
                    unlock_enter_mtx = true;
                    --waiters_;
                    break;
                }
                else
                {
                    //If it is a NOTIFY_ALL command, all threads should return
                    //from do_timed_wait function. Decrement wait count.
                    unlock_enter_mtx = 0 == --waiters_;
                    //Check if this is the last thread of notify_all waiters
                    //Only the last thread will release the mutex
                    if ( unlock_enter_mtx)
                    {
                        expected = NOTIFY_ALL;
                        cmd_.compare_exchange_strong( expected, SLEEPING);
                    }
                    break;
                }
            }
        }

        //Unlock the enter mutex if it is a single notification, if this is
        //the last notified thread in a notify_all or a timeout has occurred
        if ( unlock_enter_mtx)
            enter_mtx_.unlock();

        //Lock external again before returning from the method
        lt.lock();
        return ! timed_out;
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
