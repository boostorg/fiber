
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
    while ( LOCKED == state_)
    {
        detail::notify::ptr_t n( detail::scheduler::instance()->active() );
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
    BOOST_ASSERT( ! owner_);

    state_ = LOCKED;
    owner_ = this_fiber::get_id();
}

bool
mutex::try_lock()
{
    if ( LOCKED == state_) {
        // let other fiber release the lock
        detail::scheduler::instance()->yield();
        return false;
    }

    state_ = LOCKED;
    owner_ = this_fiber::get_id();
    return true;
}

void
mutex::unlock()
{
    BOOST_ASSERT( LOCKED == state_);
    BOOST_ASSERT( this_fiber::get_id() == owner_);

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

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
