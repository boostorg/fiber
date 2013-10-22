
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/fiber/mutex.hpp"

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

mutex::mutex() :
    owner_(),
	state_( UNLOCKED),
    splk_(),
    waiting_()
{}

mutex::~mutex()
{
    BOOST_ASSERT( ! owner_);
    BOOST_ASSERT( waiting_.empty() );
}

void
mutex::lock()
{
    detail::notify::ptr_t n( detail::scheduler::instance()->active() );
    if ( n)
    {
        for (;;)
        {
            unique_lock< detail::spinlock > lk( splk_);

            state_t expected = UNLOCKED;
            if ( state_.compare_exchange_strong( expected, LOCKED) )
                break;

            // store this fiber in order to be notified later
            waiting_.push_back( n);

            // suspend this fiber
            detail::scheduler::instance()->wait( lk);
        }
    }
    else
    {
        // notifier for main-fiber
        detail::main_notifier mn;
        n = detail::main_notifier::make_pointer( mn);

        for (;;)
        {
            unique_lock< detail::spinlock > lk( splk_);

            state_t expected = UNLOCKED;
            if ( state_.compare_exchange_strong( expected, LOCKED) )
                break;

            // store this fiber in order to be notified later
            waiting_.push_back( n);
            lk.unlock();

            // wait until main-fiber gets notified
            while ( ! n->is_ready() )
            {
                // run scheduler
                detail::scheduler::instance()->run();
            }
        }
    }

    BOOST_ASSERT( ! owner_);

    owner_ = this_fiber::get_id();
}

bool
mutex::try_lock()
{
    state_t expected = UNLOCKED;
    if ( ! state_.compare_exchange_strong( expected, LOCKED) ) {
        // let other fiber release the lock
        detail::scheduler::instance()->yield();
        return false;
    }

    owner_ = this_fiber::get_id();

    return true;
}

void
mutex::unlock()
{
    BOOST_ASSERT( LOCKED == state_);
    BOOST_ASSERT( this_fiber::get_id() == owner_);

    detail::notify::ptr_t n;
    unique_lock< detail::spinlock > lk( splk_);
    if ( ! waiting_.empty() ) {
        n.swap( waiting_.front() );
        waiting_.pop_front();
    }

    owner_ = detail::fiber_base::id();
	state_ = UNLOCKED;

    if ( n) n->set_ready();
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
