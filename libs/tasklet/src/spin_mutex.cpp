
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_TASKLET_SOURCE

#include <boost/tasklet/spin_mutex.hpp>

#include <boost/thread/thread.hpp>

#include <boost/tasklet/utility.hpp>

namespace boost {
namespace tasklets {

spin_mutex::spin_mutex() :
	state_( UNLOCKED)
{}

void
spin_mutex::lock()
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
spin_mutex::try_lock()
{
	state expected = UNLOCKED;
	return state_.compare_exchange_strong( expected, LOCKED);
}

void
spin_mutex::unlock()
{ state_.store( UNLOCKED); }

}}
