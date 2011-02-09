
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_TASKLET_SOURCE

#include "boost/tasklet/auto_reset_event.hpp"

#include <boost/thread/thread.hpp>

#include <boost/tasklet/utility.hpp>

namespace boost {
namespace tasklets {

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
		if ( this_tasklet::runs_as_tasklet() )
			this_tasklet::yield();
		else
			this_thread::yield();
		expected = SET;
	}
}

bool
auto_reset_event::try_wait()
{
	state expected = SET;
	return state_.compare_exchange_strong( expected, RESET);
}

}}
