
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/task/spin/condition.hpp"

#include <boost/thread/thread.hpp>

namespace boost {
namespace tasks {
namespace spin {

void
condition::notify_( command cmd)
{
	enter_mtx_.lock();

	if ( 0 == waiters_.load() )
	{
		enter_mtx_.unlock();
		return;
	}

	command expected = SLEEPING;
	while ( ! cmd_.compare_exchange_strong( expected, cmd) )
	{
		if ( this_tasklet::runs_as_tasklet() )
			this_tasklet::yield();
		else
			this_thread::yield();
		expected = SLEEPING;
	}
}

condition::condition() :
	cmd_( SLEEPING),
	waiters_( 0),
	enter_mtx_(),
	check_mtx_()
{}

void
condition::notify_one()
{ notify_( NOTIFY_ONE); }

void
condition::notify_all()
{ notify_( NOTIFY_ALL); }

}}}
