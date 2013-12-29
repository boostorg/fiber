
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/fiber/timed_mutex.hpp"

#include <algorithm>

#include <boost/assert.hpp>

#include "boost/fiber/detail/scheduler.hpp"
#include "boost/fiber/interruption.hpp"
#include "boost/fiber/operations.hpp"

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

timed_mutex::timed_mutex() :
    splk_(),
	state_( UNLOCKED),
    owner_(),
    waiting_()
{}

timed_mutex::~timed_mutex()
{
    BOOST_ASSERT( ! owner_);
    BOOST_ASSERT( waiting_.empty() );
}

void
timed_mutex::lock()
{
    detail::notify::ptr_t n( detail::scheduler::instance()->active() );
    if ( n)
    {
        for (;;)
        {
            unique_lock< detail::spinlock > lk( splk_);

            if ( UNLOCKED == state_)
            {
                state_ = LOCKED;
                BOOST_ASSERT( ! owner_);
                owner_ = this_fiber::get_id();
                return;
            }

            // store this fiber in order to be notified later
            BOOST_ASSERT( waiting_.end() == std::find( waiting_.begin(), waiting_.end(), n) );
            waiting_.push_back( n);

            // suspend this fiber
            detail::scheduler::instance()->wait( lk);
        }
    }
    else
    {
        for (;;)
        {
            // local notification for main-fiber
            n = detail::scheduler::instance()->get_main_notifier();

            unique_lock< detail::spinlock > lk( splk_);

            if ( UNLOCKED == state_)
            {
                state_ = LOCKED;
                BOOST_ASSERT( ! owner_);
                owner_ = this_fiber::get_id();
                return;
            }

            // store this fiber in order to be notified later
            BOOST_ASSERT( waiting_.end() == std::find( waiting_.begin(), waiting_.end(), n) );
            waiting_.push_back( n);
            lk.unlock();

            // wait until main-fiber gets notified
            while ( ! n->is_ready() )
                // run scheduler
                detail::scheduler::instance()->run();
        }
    }
}

bool
timed_mutex::try_lock()
{
    unique_lock< detail::spinlock > lk( splk_);

    if ( UNLOCKED == state_)
    {
        state_ = LOCKED;
        BOOST_ASSERT( ! owner_);
        owner_ = this_fiber::get_id();
        return true;
    }
    else
    {
        lk.unlock();
        // let other fiber release the lock
        this_fiber::yield();
        return false;
    }
}

bool
timed_mutex::try_lock_until( clock_type::time_point const& timeout_time)
{
    detail::notify::ptr_t n( detail::scheduler::instance()->active() );
    if ( n)
    {
        for (;;)
        {
            unique_lock< detail::spinlock > lk( splk_);

            if ( clock_type::now() > timeout_time)
                return false;

            if ( UNLOCKED == state_)
            {
                state_ = LOCKED;
                BOOST_ASSERT( ! owner_);
                owner_ = this_fiber::get_id();
                return true;
            }

            // store this fiber in order to be notified later
            BOOST_ASSERT( waiting_.end() == std::find( waiting_.begin(), waiting_.end(), n) );
            waiting_.push_back( n);

            // suspend this fiber until notified or timed-out
            if ( ! detail::scheduler::instance()->wait_until( timeout_time, lk) )
            {
                lk.lock();
                // remove fiber from waiting-list
                waiting_.erase(
                    std::find( waiting_.begin(), waiting_.end(), n) );
                lk.unlock();
                return false;
            }
        }
    }
    else
    {
        for (;;)
        {
            // local notification for main-fiber
            n = detail::scheduler::instance()->get_main_notifier();

            unique_lock< detail::spinlock > lk( splk_);

            if ( clock_type::now() > timeout_time)
                return false;

            if ( UNLOCKED == state_)
            {
                state_ = LOCKED;
                BOOST_ASSERT( ! owner_);
                owner_ = this_fiber::get_id();
                return true;
            }

            // store this fiber in order to be notified later
            BOOST_ASSERT( waiting_.end() == std::find( waiting_.begin(), waiting_.end(), n) );
            waiting_.push_back( n);
            lk.unlock();

            // wait until main-fiber gets notified
            while ( ! n->is_ready() )
            {
                if ( clock_type::now() > timeout_time)
                {
                    lk.lock();
                    // remove fiber from waiting-list
                    waiting_.erase(
                            std::find( waiting_.begin(), waiting_.end(), n) );
                    lk.unlock();
                    return false;
                }
                // run scheduler
                detail::scheduler::instance()->run();
            }
        }
    }
}

void
timed_mutex::unlock()
{
    BOOST_ASSERT( LOCKED == state_);
    BOOST_ASSERT( this_fiber::get_id() == owner_);

    unique_lock< detail::spinlock > lk( splk_);
    detail::notify::ptr_t n;
    
    if ( ! waiting_.empty() )
    {
        n.swap( waiting_.front() );
        waiting_.pop_front();
    }
    owner_ = detail::fiber_base::id();
    state_ = UNLOCKED;
    lk.unlock();
    if ( n) n->set_ready();
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
