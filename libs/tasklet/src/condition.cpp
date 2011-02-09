
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_TASKLET_SOURCE

#include "boost/tasklet/condition.hpp"

#include <boost/foreach.hpp>

namespace boost {
namespace tasklets {

condition::condition() :
	oid_( this),
	waiting_tasklets_(),
	oidx_( waiting_tasklets_.get< ordered_idx_tag >() ),
	sidx_( waiting_tasklets_.get< sequenced_idx_tag >() ),
	cmd_( SLEEPING),
	waiters_( 0),
	enter_mtx_(),
	check_mtx_(),
	mtx_()
{}

condition::~condition()
{
	BOOST_ASSERT( waiting_tasklets_.empty() );
	BOOST_ASSERT( 0 == waiters_.load() );
}

void
condition::notify_one()
{
	enter_mtx_.lock();

	if ( 0 == waiters_.load() )
	{
		enter_mtx_.unlock();
		return;
	}

	command expected = SLEEPING;
	while ( ! cmd_.compare_exchange_strong( expected, NOTIFY_ONE) )
	{
		if ( this_tasklet::runs_as_tasklet() )
			this_tasklet::yield();
		else
			this_thread::yield();
		expected = SLEEPING;
	}
	
	spin_mutex::scoped_lock lk( mtx_);
	tasklet f( * sidx_.begin() ); 
	sidx_.pop_front();
	BOOST_ASSERT( f.impl_->attached_strategy() );
	f.impl_->attached_strategy()->object_notify_one( oid_);
}

void
condition::notify_all()
{
	enter_mtx_.lock();

	if ( 0 == waiters_.load() )
	{
		enter_mtx_.unlock();
		return;
	}

	command expected = SLEEPING;
	while ( ! cmd_.compare_exchange_strong( expected, NOTIFY_ALL) )
	{
		if ( this_tasklet::runs_as_tasklet() )
			this_tasklet::yield();
		else
			this_thread::yield();
		expected = SLEEPING;
	}

	spin_mutex::scoped_lock lk( mtx_);
	BOOST_FOREACH( tasklet f, sidx_)
	{
		BOOST_ASSERT( f.impl_->attached_strategy() );
		f.impl_->attached_strategy()->object_notify_all( oid_);
	}
	waiting_tasklets_.clear();
}

}}
