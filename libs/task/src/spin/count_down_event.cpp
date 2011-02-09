
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/task/spin/count_down_event.hpp"

#include <boost/thread/thread.hpp>

#include <boost/task/spin/mutex.hpp>
#include <boost/task/utility.hpp>

namespace boost {
namespace tasks {
namespace spin {

count_down_event::count_down_event( std::size_t initial) :
	initial_( initial),
	current_( initial_)
{}

std::size_t
count_down_event::initial() const
{ return initial_; }

std::size_t
count_down_event::current() const
{ return current_.load(); }

bool
count_down_event::is_set() const
{ return 0 == current_.load(); }

void
count_down_event::set()
{
	for (;;)
	{
		if ( 0 == current_.load() )
			return;
		std::size_t expected = current_.load();
		if ( current_.compare_exchange_strong( expected, expected - 1) )
			return;
	}
}

void
count_down_event::wait()
{
	while ( 0 != current_.load() )
	{
		this_thread::interruption_point();
		if ( this_task::runs_in_pool() )
			this_task::yield();
		else
			this_thread::yield();
		this_thread::interruption_point();
	}
}

bool
count_down_event::timed_wait( system_time const& abs_time)
{
	if ( get_system_time() >= abs_time) return false;

	while ( 0 != current_.load() )
	{
		this_thread::interruption_point();
		if ( this_task::runs_in_pool() )
			this_task::yield();
		else
			this_thread::yield();
		this_thread::interruption_point();

		if ( get_system_time() >= abs_time) return false;
	}

	return true;
}

}}}
