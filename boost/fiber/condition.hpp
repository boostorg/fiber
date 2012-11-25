
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
#include <boost/chrono/system_clocks.hpp>
#include <boost/config.hpp>
#include <boost/thread/locks.hpp>
#include <boost/utility.hpp>

#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/scheduler.hpp>
#include <boost/fiber/detail/fiber_base.hpp>
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

    command                 cmd_;
    std::size_t             waiters_;
    mutex                   enter_mtx_;
    mutex                   check_mtx_;
    std::deque<
        detail::fiber_base::ptr_t
    >                       waiting_;

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
        {
            mutex::scoped_lock lk( enter_mtx_);
            BOOST_ASSERT( lk);
            ++waiters_;
            lt.unlock();
        }

        bool unlock_enter_mtx = false;
        for (;;)
        {
            while ( SLEEPING == cmd_)
            {
                if ( this_fiber::is_fiberized() )
                {
                    waiting_.push_back(
                        detail::scheduler::instance().active() );
                    detail::scheduler::instance().wait();
                }
                else
                    detail::scheduler::instance().run();
            }

            if ( NOTIFY_ONE == cmd_)
            {
                unlock_enter_mtx = true;
                --waiters_;
                cmd_ = SLEEPING;
                break;
            }
            else
            {
                unlock_enter_mtx = 0 == --waiters_;
                if ( unlock_enter_mtx)
                    cmd_ = SLEEPING;
                break;
            }
        }

        if ( unlock_enter_mtx)
            enter_mtx_.unlock();

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
        if ( (chrono::system_clock::time_point::max)() == abs_time){
            wait( lt);
            return true;
        }
        chrono::system_clock::time_point now( chrono::system_clock::now() );
        if ( now >= abs_time) return false;

        {
            mutex::scoped_lock lk( enter_mtx_);
            BOOST_ASSERT( lk);
            ++waiters_;
            lt.unlock();
        }

        bool unlock_enter_mtx = false, timed_out = false;
        for (;;)
        {
            while ( SLEEPING == cmd_)
            {
                now = chrono::system_clock::now();
                if ( now >= abs_time)
                {
                    while ( ! ( timed_out = enter_mtx_.try_lock() ) )
                        detail::scheduler::instance().yield();
                    break; 
                }

                if ( this_fiber::is_fiberized() )
                {
                    waiting_.push_back(
                        detail::scheduler::instance().active() );
                    detail::scheduler::instance().sleep( abs_time);
                }
                else
                    detail::scheduler::instance().run();

                now = chrono::system_clock::now();
                if ( now >= abs_time)
                {
                    while ( ! ( timed_out = enter_mtx_.try_lock() ) )
                        detail::scheduler::instance().yield();
                    break; 
                }
            }

            if ( timed_out)
            {
                unlock_enter_mtx = true;
                --waiters_;
                break;
            }

            if ( NOTIFY_ONE == cmd_)
            {
                unlock_enter_mtx = true;
                --waiters_;
                cmd_ = SLEEPING;
                break;
            }
            else
            {
                unlock_enter_mtx = 0 == --waiters_;
                if ( unlock_enter_mtx)
                    cmd_ = SLEEPING;
                break;
            }
        }

        if ( unlock_enter_mtx)
            enter_mtx_.unlock();

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
