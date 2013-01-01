
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_FIBERS_SOURCE

#include <boost/fiber/detail/spin_mutex.hpp>

#include <boost/assert.hpp>
#include <boost/fiber/operations.hpp>

namespace boost {
namespace fibers {
namespace detail {

spin_mutex::spin_mutex() :
	state_( UNLOCKED)
{}

void
spin_mutex::lock()
{
    while ( LOCKED == state_.exchange( LOCKED, memory_order_acquire) )
    {
        // busy-wait
		BOOST_ASSERT( this_fiber::is_fiberized() );
		this_fiber::yield();
    }
}

bool
spin_mutex::timed_lock( chrono::system_clock::time_point const& abs_time)
{
	if ( chrono::system_clock::now() >= abs_time)
		return false;

	for (;;)
	{
		if ( try_lock() ) break;

		if ( chrono::system_clock::now() >= abs_time)
			return false;

		//this_fiber::interruption_point();
        //FIXME: what to do if not a fiber
		BOOST_ASSERT( this_fiber::is_fiberized() );
		this_fiber::yield();
		//this_fiber::interruption_point();
	}

	return true;
}

bool
spin_mutex::try_lock()
{ return UNLOCKED == state_.exchange( LOCKED, memory_order_acquire); }

void
spin_mutex::unlock()
{ state_ = UNLOCKED; }

}}}
