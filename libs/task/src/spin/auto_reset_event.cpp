
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/task/spin/auto_reset_event.hpp"

#include <boost/thread/thread.hpp>

#include <boost/task/utility.hpp>

namespace boost {
namespace tasks {
namespace spin {

auto_reset_event::auto_reset_event( bool isset) :
	state_( isset ? SET : RESET)
{}

void
auto_reset_event::set()
{ state_.store( SET); }

void
auto_reset_event::wait()
{
	state expected = SET;
	while ( ! state_.compare_exchange_strong( expected, RESET) )
	{
		this_thread::interruption_point();
		if ( this_task::runs_in_pool() )
			this_task::yield();
		else
			this_thread::yield();	
		this_thread::interruption_point();

		expected = SET;
	}
}

bool
auto_reset_event::try_wait()
{
	state expected = SET;
	return state_.compare_exchange_strong( expected, RESET);
}

bool
auto_reset_event::timed_wait( system_time const& abs_time)
{
	if ( get_system_time() >= abs_time) return false;

	state expected = SET;
	while ( ! state_.compare_exchange_strong( expected, RESET) )
	{
		this_thread::interruption_point();
		if ( this_task::runs_in_pool() )
			this_task::yield();
		else
			this_thread::yield();	
		this_thread::interruption_point();
	
		if ( get_system_time() >= abs_time) return false;
		expected = SET;
	}

	return true;
}

}}}
