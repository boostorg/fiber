
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_TASKLET_SOURCE

#include <boost/tasklet/mutex.hpp>

#include <boost/assert.hpp>
#include <boost/thread/thread.hpp>

#include <boost/tasklet/utility.hpp>

namespace boost {
namespace tasklets {

mutex::mutex() :
	oid_( this),
	state_( UNLOCKED),
	mtx_(),
	waiting_tasklets_(),
	oidx_( waiting_tasklets_.get< ordered_idx_tag >() ),
	sidx_( waiting_tasklets_.get< sequenced_idx_tag >() )
{}

mutex::~mutex()
{ BOOST_ASSERT( waiting_tasklets_.empty() ); }

void
mutex::lock()
{
	for (;;)
	{
		spin_mutex::scoped_lock lk( mtx_);
		if ( UNLOCKED == state_)
		{
			state_ = LOCKED;
			// TODO: store identifier of tasklet/thread
			lk.unlock();
			break;
		}
		else if ( this_tasklet::runs_as_tasklet() )
		{
			tasklet * f( strategy::active_tasklet);
			BOOST_ASSERT( f);
			oidx_.insert( * f);
			BOOST_ASSERT( f->impl_->attached_strategy() );
			f->impl_->attached_strategy()->wait_for_object( oid_, lk);
		}
		else
		{
			lk.unlock();
			this_thread::yield();
		}
	}
}

bool
mutex::try_lock()
{
	spin_mutex::scoped_lock lk( mtx_);
	if ( LOCKED == state_) return false;
	state_ = LOCKED;
	return true;
}

void
mutex::unlock()
{
	// TODO: only the tasklet/thread locked the mutex
	//       can call unlock()
	spin_mutex::scoped_lock lk( mtx_);
	state_ = UNLOCKED;
	if ( waiting_tasklets_.empty() ) return;
	tasklet f( * sidx_.begin() ); 
	sidx_.pop_front();
	BOOST_ASSERT( f.impl_->attached_strategy() );
	f.impl_->attached_strategy()->object_notify_one( oid_);
}

}}
