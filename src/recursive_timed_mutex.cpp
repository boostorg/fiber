
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/fiber/recursive_timed_mutex.hpp"

#include <algorithm>

#include <boost/assert.hpp>

#include "boost/fiber/detail/main_notifier.hpp"
#include "boost/fiber/detail/scheduler.hpp"
#include "boost/fiber/interruption.hpp"
#include "boost/fiber/operations.hpp"

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

recursive_timed_mutex::recursive_timed_mutex() :
    owner_(),
    count_( 0),
	state_( UNLOCKED),
    waiting_()
{}

recursive_timed_mutex::~recursive_timed_mutex()
{
    BOOST_ASSERT( ! owner_);
    BOOST_ASSERT( 0 == count_);
    BOOST_ASSERT( waiting_.empty() );
}

void
recursive_timed_mutex::lock()
{
    if ( LOCKED == state_ && this_fiber::get_id() == owner_)
    {
        ++count_;
        return;
    }

    while ( LOCKED == state_)
    {
        detail::notify::ptr_t n( detail::scheduler::instance()->active() );
        try
        {
            if ( n)
            {
                // store this fiber in order to be notified later
                waiting_.push_back( n);

                // suspend this fiber
                detail::scheduler::instance()->wait();
            }
            else
            {
                // notifier for main-fiber
                detail::main_notifier mn;
                n = detail::main_notifier::make_pointer( mn);

                // store this fiber in order to be notified later
                waiting_.push_back( n);

                // wait until main-fiber gets notified
                while ( ! n->is_ready() )
                {
                    // run scheduler
                    detail::scheduler::instance()->run();
                }
            }
        }
        catch (...)
        {
            // remove fiber from waiting_
            waiting_.erase(
                std::find( waiting_.begin(), waiting_.end(), n) );
            throw;
        }
    }
    BOOST_ASSERT( ! owner_);
    BOOST_ASSERT( 0 == count_);

    state_ = LOCKED;
    owner_ = this_fiber::get_id();
    ++count_;
}

bool
recursive_timed_mutex::try_lock()
{
    if ( LOCKED == state_)
    {
        if ( this_fiber::get_id() == owner_)
        {
            ++count_;
            return true;
        }
        else
        {
            // let other fiber release the lock
            detail::scheduler::instance()->yield();
            return false;
        }
    }

    state_ = LOCKED;
    owner_ = this_fiber::get_id();
    ++count_;
    return true;
}

bool
recursive_timed_mutex::try_lock_until( clock_type::time_point const& timeout_time)
{
    if ( LOCKED == state_ && this_fiber::get_id() == owner_)
    {
        ++count_;
        return true;
    }

    while ( LOCKED == state_ && clock_type::now() < timeout_time)
    {
        detail::notify::ptr_t n( detail::scheduler::instance()->active() );
        try
        {
            if ( n)
            {
                // store this fiber in order to be notified later
                waiting_.push_back( n);

                // suspend this fiber until notified or timed-out
                if ( ! detail::scheduler::instance()->wait_until( timeout_time) )
                    // remove fiber from waiting-list
                    waiting_.erase(
                        std::find( waiting_.begin(), waiting_.end(), n) );
            }
            else
            {
                // notifier for main-fiber
                detail::main_notifier mn;
                n = detail::main_notifier::make_pointer( mn);

                // store this fiber in order to be notified later
                waiting_.push_back( n);

                // wait until main-fiber gets notified
                while ( ! n->is_ready() )
                {
                    if ( ! ( clock_type::now() < timeout_time) )
                    {
                        // remove fiber from waiting-list
                        waiting_.erase(
                            std::find( waiting_.begin(), waiting_.end(), n) );
                        break;
                    }
                    // run scheduler
                    detail::scheduler::instance()->run();
                }
            }
        }
        catch (...)
        {
            // remove fiber from waiting-list
            waiting_.erase(
                std::find( waiting_.begin(), waiting_.end(), n) );
            throw;
        }
    }

    if ( LOCKED == state_) return false;

    BOOST_ASSERT( ! owner_);
    BOOST_ASSERT( 0 == count_);

    state_ = LOCKED;
    owner_ = this_fiber::get_id();
    ++count_;

    return true;
}

void
recursive_timed_mutex::unlock()
{
    BOOST_ASSERT( LOCKED == state_);
    BOOST_ASSERT( this_fiber::get_id() == owner_);
    
    if ( 0 == --count_)
    {
        detail::notify::ptr_t n;

        if ( ! waiting_.empty() ) {
            n.swap( waiting_.front() );
            waiting_.pop_front();
        }

        state_ = UNLOCKED;
        owner_ = detail::fiber_base::id();

        if ( n)
            n->set_ready();
    }
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
