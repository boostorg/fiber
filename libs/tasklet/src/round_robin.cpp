
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_TASKLET_SOURCE

#include <boost/tasklet/round_robin.hpp>

#include <memory>
#include <utility>

#include <boost/assert.hpp>
#include <boost/foreach.hpp>
#include <boost/thread/detail/move.hpp>

#include <boost/tasklet/exceptions.hpp>

#include <boost/config/abi_prefix.hpp>

namespace boost {
namespace tasklets {

round_robin::round_robin() :
	mtx_(),
	tasklets_(),
	objects_(),
	runnable_tasklets_(),
	terminated_tasklets_()
{}

void
round_robin::add( tasklet & t)
{
	spin_mutex::scoped_lock lk( mtx_);

	if ( ! t) throw tasklet_moved();

	BOOST_ASSERT( in_state_not_started( t) );

	// set state to ready
	set_state_ready( t);

	// attach to this scheduler
	attach( t);

	// insert tasklet to tasklet-list
	std::pair< std::map< tasklet::id, schedulable >::iterator, bool > result(
		tasklets_.insert(
			std::make_pair(
				t.get_id(),
				schedulable( t) ) ) );

	// check result
	if ( ! result.second) throw scheduler_error("inserting tasklet failed");

	// put tasklet to runnable-queue
	runnable_tasklets_.push_back( result.first->first);
}

void
round_robin::join( tasklet & t)
{
	spin_mutex::scoped_lock lk( mtx_);

	BOOST_ASSERT( t);
	BOOST_ASSERT( ! in_state_not_started( t) );
	BOOST_ASSERT( active_tasklet);
	tasklet_map::iterator i = tasklets_.find( t.get_id() );
	if ( i == tasklets_.end() ) return;
	schedulable s( i->second);

	// nothing to do for a terminated tasklet
	if ( in_state_terminated( t) ) return;

	// prevent self-join
	if ( active_tasklet->get_id() == t.get_id() )
		throw scheduler_error("self-join denied");

	// register on tasklet to be joined
	tasklets_[t.get_id()].joining_tasklets.push_back( active_tasklet->get_id() );

	// set state waiting
	set_state_wait_for_tasklet( * active_tasklet);

	// set tasklet-id waiting-on
	tasklets_[active_tasklet->get_id()].waiting_on_tasklet = t.get_id();

	// switch to master-tasklet
	lk.unlock();
	strategy::yield( * active_tasklet);
	lk.lock();

	// tasklet returned
	BOOST_ASSERT( in_state_running( * active_tasklet) );
	BOOST_ASSERT( ! tasklets_[active_tasklet->get_id()].waiting_on_tasklet);

	// check if interruption was requested
	if ( interruption_enabled( * active_tasklet) )
		throw tasklet_interrupted();
}

void
round_robin::interrupt( tasklet & t)
{
	spin_mutex::scoped_lock lk( mtx_);

	BOOST_ASSERT( t);
	BOOST_ASSERT( ! in_state_not_started( t) );
	tasklet_map::iterator i = tasklets_.find( t.get_id() );
	if ( i == tasklets_.end() ) return;
	schedulable s( i->second);

	// nothing to do for al terminated tasklet
	if ( in_state_terminated( t) ) return;

	enable_interruption( t);

	// if tasklet is waiting
	if ( in_state_wait_for_tasklet( t) )
	{
		// tasklet is waiting (joining) on another tasklet
		// remove it from the waiting-queue, reset waiting-on
		// and reset the waiting state
		BOOST_ASSERT( s.waiting_on_tasklet);
		tasklets_[* s.waiting_on_tasklet].joining_tasklets.remove( t.get_id() );
		tasklets_[t.get_id()].waiting_on_tasklet.reset();
		set_state_ready( t);
		runnable_tasklets_.push_back( t.get_id() );
	} else if ( in_state_wait_for_object( t) ) {
		// tasklet is waiting on an object
		// remove it from the waiting-queue, reset waiting-on
		// and reset the waiting state
		BOOST_ASSERT( s.waiting_on_object);
		objects_[* s.waiting_on_object].remove( t.get_id() );
		tasklets_[t.get_id()].waiting_on_object.reset();
		set_state_ready( t);
		runnable_tasklets_.push_back( t.get_id());
	}
}	

void
round_robin::reschedule( tasklet & t)
{
	spin_mutex::scoped_lock lk( mtx_);

	BOOST_ASSERT( t);
	BOOST_ASSERT( ! in_state_not_started( t) );
	BOOST_ASSERT( ! in_state_terminated( t) );
	tasklet_map::iterator i = tasklets_.find( t.get_id() );
	if ( i == tasklets_.end() ) return;
}

void
round_robin::cancel( tasklet & t)
{
	spin_mutex::scoped_lock lk( mtx_);

	BOOST_ASSERT( t);
	BOOST_ASSERT( ! in_state_not_started( t) );
	BOOST_ASSERT( active_tasklet);

	// nothing to do for al terminated tasklet
	if ( in_state_terminated( t) ) return;

	tasklet_map::iterator i( tasklets_.find( t.get_id() ) );
	if ( i == tasklets_.end() ) return;
	schedulable s( i->second);

	// invoke each tasklet waiting on this tasklet
	BOOST_FOREACH( tasklet::id id__, s.joining_tasklets)
	{
		schedulable s__( tasklets_[id__]);
		tasklet f__( s__.f);
		BOOST_ASSERT( s__.waiting_on_tasklet);
		BOOST_ASSERT( in_state_wait_for_tasklet( f__) );

		// clear waiting-on
		tasklets_[id__].waiting_on_tasklet.reset();

		// put tasklet on runnable-queue
		set_state_ready( f__);
		runnable_tasklets_.push_back( id__);
	}
	// clear waiting-queue
	tasklets_[t.get_id()].joining_tasklets.clear();

	// if tasklet is ready remove it from the runnable-queue
	// and put it to terminated-queue
	if ( in_state_ready( t) )
	{
		set_state_terminated( t);
		runnable_tasklets_.remove( t.get_id() );
		terminated_tasklets_.push( t.get_id() );	
	}
	// if tasklet is running (== active tasklet)
	// reset active tasklet
	// put it to terminated-queue and switch
	// to master tasklet
	else if ( in_state_running( t) )
	{
		BOOST_ASSERT( active_tasklet->get_id() == t.get_id() );
		set_state_terminated( t);
		terminated_tasklets_.push( t.get_id() );
		lk.unlock();
		strategy::yield( t);
	}
	// if tasklet is waiting then remove it from the
	// waiting-queue and put it to terminated-queue
	else if ( in_state_wait_for_tasklet( t) )
	{
		BOOST_ASSERT( s.waiting_on_tasklet);
		set_state_terminated( t);
		tasklets_[* s.waiting_on_tasklet].joining_tasklets.remove( t.get_id() );
		terminated_tasklets_.push( t.get_id() );	
	}
	else
		BOOST_ASSERT( ! "should never reached");
}

void
round_robin::yield()
{
	spin_mutex::scoped_lock lk( mtx_);

	BOOST_ASSERT( active_tasklet);
	BOOST_ASSERT( in_state_running( * active_tasklet) );
	BOOST_ASSERT( ! tasklets_[active_tasklet->get_id()].waiting_on_tasklet);

	// set state ready
	set_state_ready( * active_tasklet);

	// put tasklet to runnable-queue
	runnable_tasklets_.push_back( active_tasklet->get_id() );

	// switch to master-tasklet
	lk.unlock();
	strategy::yield( * active_tasklet);
}

void
round_robin::wait_for_object( object::id const& oid, spin_mutex::scoped_lock & slk)
{
	spin_mutex::scoped_lock lk( mtx_);

	BOOST_ASSERT( active_tasklet);
	tasklet::id id = active_tasklet->get_id();
	tasklet_map::iterator i = tasklets_.find( id);
	if ( i == tasklets_.end() ) return;
	schedulable s( i->second);
	tasklet f( s.f);
	BOOST_ASSERT( f);
	BOOST_ASSERT( active_tasklet->get_id() == id);
	BOOST_ASSERT( in_state_running( f) );
	BOOST_ASSERT( ! s.waiting_on_tasklet);
	BOOST_ASSERT( ! s.waiting_on_object);

	// register on object to be waiting on
	objects_[oid].push_back( id);
	
	// set state waiting
	set_state_wait_for_object( f);
	
	// set object-id waiting-on
	tasklets_[id].waiting_on_object = oid;

	// release lock protecting sync. primitive
	slk.unlock();

	// switch to master-tasklet
	lk.unlock();
	strategy::yield( * active_tasklet);
	lk.lock();

	// tasklet returned
	BOOST_ASSERT( active_tasklet->get_id() == id);
	BOOST_ASSERT( in_state_running( tasklets_[id].f) );
	BOOST_ASSERT( ! tasklets_[id].waiting_on_tasklet);
	BOOST_ASSERT( ! tasklets_[id].waiting_on_object);

	// check if interruption was requested
	if ( interruption_enabled( f) )
		throw tasklet_interrupted();
}

void
round_robin::object_notify_one( object::id const& oid)
{
	spin_mutex::scoped_lock lk( mtx_);

	object_map::iterator oi( objects_.find( oid) );
	BOOST_ASSERT( oi != objects_.end() );
	if ( oi->second.empty() ) return;
	tasklet::id id( oi->second.front() );
	oi->second.pop_front();

	tasklet_map::iterator fi = tasklets_.find( id);
	BOOST_ASSERT( fi != tasklets_.end() );
	schedulable s( fi->second);
	tasklet f( s.f);
	BOOST_ASSERT( f);
	BOOST_ASSERT( ! s.waiting_on_tasklet);
	BOOST_ASSERT( s.waiting_on_object);
	BOOST_ASSERT( * s.waiting_on_object == oid);

	// remove object waiting for
	tasklets_[id].waiting_on_object.reset();
	
	// set state ready
	set_state_ready( f);

	// put tasklet to runnable-queue
	runnable_tasklets_.push_back( id);
}

void
round_robin::object_notify_all( object::id const& oid)
{
	spin_mutex::scoped_lock lk( mtx_);

	object_map::iterator oi( objects_.find( oid) );
	if ( oi == objects_.end() )
		// NOTE: because of previous calls to this function
		//       all references may be already released
		//       this can happen by calling condition::notify_all()
		return;
	tasklet_id_list::iterator fe( oi->second.end() );
	for (
			tasklet_id_list::iterator fi( oi->second.begin() );
			fi != fe;
			++fi)
	{
		tasklet::id id( * fi);

		tasklet_map::iterator i = tasklets_.find( id);
		BOOST_ASSERT( i != tasklets_.end() );
		schedulable s( i->second);
		tasklet f( s.f);
		BOOST_ASSERT( f);
		BOOST_ASSERT( ! s.waiting_on_tasklet);
		BOOST_ASSERT( s.waiting_on_object);
		BOOST_ASSERT( * s.waiting_on_object == oid);

		// remove object waiting for
		tasklets_[id].waiting_on_object.reset();
		
		// set state ready
		set_state_ready( f);

		// put tasklet to runnable-queue
		runnable_tasklets_.push_back( id);
	}
	objects_.erase( oi);
}

void
round_robin::release( tasklet & t)
{
	spin_mutex::scoped_lock lk( mtx_);

	BOOST_ASSERT( t);
	tasklet_map::iterator fi = tasklets_.find( t.get_id() );
	if ( fi == tasklets_.end() )
		throw scheduler_error("tasklet not managed by scheduler");
	schedulable s( fi->second);
	if ( ! in_state_ready( t) || ! s.joining_tasklets.empty() )
		throw tasklet_error("tasklet can not be released");
	BOOST_ASSERT( ! s.waiting_on_tasklet);
	BOOST_ASSERT( ! s.waiting_on_object);

	runnable_tasklets_.remove( t.get_id() );
	tasklets_.erase( t.get_id() );
}

void
round_robin::migrate( tasklet  & t)
{
	spin_mutex::scoped_lock lk( mtx_);

	if ( ! t) throw tasklet_moved();

	BOOST_ASSERT( in_state_ready( t) );

	// attach to this scheduler
	attach( t);

	// insert tasklet to tasklet-list
	std::pair< std::map< tasklet::id, schedulable >::iterator, bool > result(
		tasklets_.insert(
			std::make_pair(
				t.get_id(),
				schedulable( t) ) ) );

	// check result
	if ( ! result.second) throw scheduler_error("migrating tasklet failed");

	// put tasklet to runnable-queue
	runnable_tasklets_.push_back( result.first->first);
}

void
round_robin::detach_all()
{
	spin_mutex::scoped_lock lk( mtx_);
	BOOST_FOREACH( tasklet_map::value_type va, tasklets_)
	{ detach( va.second.f); }
}

bool
round_robin::run()
{
	spin_mutex::scoped_lock lk( mtx_);

	bool result( false);
	if ( ! runnable_tasklets_.empty() )
	{
		schedulable s = tasklets_[runnable_tasklets_.front()];
		runnable_tasklets_.pop_front();
		active_tasklet =  & s.f;
		BOOST_ASSERT( ! s.waiting_on_tasklet);
		BOOST_ASSERT( ! s.waiting_on_object);
		BOOST_ASSERT( in_state_ready( * active_tasklet) );
		set_state_running( * active_tasklet);
		lk.unlock();
		call( * active_tasklet);
		lk.lock();
		active_tasklet = 0;
		result = true;
	}

	while ( ! terminated_tasklets_.empty() )
	{
		tasklet_map::iterator i( tasklets_.find( terminated_tasklets_.front() ) );
		BOOST_ASSERT( i != tasklets_.end() );
		schedulable s( i->second);
		tasklet f( s.f);
		terminated_tasklets_.pop();
		BOOST_ASSERT( s.joining_tasklets.empty() );	
		BOOST_ASSERT( ! s.waiting_on_tasklet);
		BOOST_ASSERT( ! s.waiting_on_object);
		BOOST_ASSERT( in_state_terminated( f) );	
		tasklets_.erase( i);
	}
	return result;
}

bool
round_robin::empty() const
{
	spin_mutex::scoped_lock lk( mtx_);
	return tasklets_.empty();
}

std::size_t
round_robin::size() const
{
	spin_mutex::scoped_lock lk( mtx_);
	return tasklets_.size();
}

std::size_t
round_robin::ready() const
{
	spin_mutex::scoped_lock lk( mtx_);
	return runnable_tasklets_.size();
}

bool
round_robin::has_ready() const
{
	spin_mutex::scoped_lock lk( mtx_);
	return ! runnable_tasklets_.empty();
}

}}

#include <boost/config/abi_suffix.hpp>
