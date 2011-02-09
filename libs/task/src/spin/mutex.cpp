
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/task/spin/mutex.hpp>

#include <boost/thread/thread.hpp>

#include <boost/task/utility.hpp>

namespace boost {
namespace tasks {
namespace spin {

mutex::mutex() :
	state_( UNLOCKED)
{}

void
mutex::lock()
{
	for (;;)
	{
		state expected = UNLOCKED;
		if ( state_.compare_exchange_strong( expected, LOCKED) )
			break;
		if ( this_tasklet::runs_as_tasklet() )
			this_tasklet::yield();
		else
			this_thread::yield();
	}
}

bool
mutex::timed_lock( system_time const& abs_time)
{
	if ( abs_time.is_infinity() )
	{
		lock();
		return true;
	}

	if ( get_system_time() >= abs_time)
		return false;

	for (;;)
	{
		if ( try_lock() ) break;

		if ( get_system_time() >= abs_time)
			return false;

		this_thread::interruption_point();
		if ( this_task::runs_in_pool() )
			this_task::yield();
		else
			this_thread::yield();	
		this_thread::interruption_point();
	}

	return true;
}

bool
mutex::try_lock()
{
	state expected = UNLOCKED;
	return state_.compare_exchange_strong( expected, LOCKED);
}

void
mutex::unlock()
{ state_.store( UNLOCKED); }

}}}
