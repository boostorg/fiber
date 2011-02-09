
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_TASKLET_SOURCE

#include <boost/tasklet/strategy.hpp>

#include <utility>

#include <boost/assert.hpp>

#include <boost/tasklet/detail/tasklet_base.hpp>
#include <boost/tasklet/detail/tasklet_object.hpp>
#include <boost/tasklet/detail/state_flags.hpp>
#include <boost/tasklet/exceptions.hpp>

namespace boost {
namespace tasklets {

BOOST_TASKLET_TSSDECL tasklet * strategy::active_tasklet = 0;

bool
strategy::runs_as_tasklet_()
{ return 0 != active_tasklet; }

tasklet::id
strategy::get_id_()
{
	if ( ! active_tasklet) throw tasklet_error("not a tasklet");
	return active_tasklet->get_id();
}

void
strategy::interruption_point_()
{
	if ( ! active_tasklet) throw tasklet_error("not a tasklet");
	if ( detail::INTERRUPTION_ENABLED == active_tasklet->impl_->interrupt() )
		throw tasklet_interrupted();
}

bool
strategy::interruption_requested_()
{
	if ( ! active_tasklet) throw tasklet_error("not a tasklet");
	return active_tasklet->interruption_requested();
}

detail::interrupt_type &
strategy::interrupt_flags_()
{
	if ( ! active_tasklet) throw tasklet_error("not a tasklet");
	return active_tasklet->impl_->interrupt();
}

bool
strategy::interruption_enabled_()
{
	if ( ! active_tasklet) throw tasklet_error("not a tasklet");
	return ( active_tasklet->impl_->interrupt() & detail::INTERRUPTION_ENABLED) != 0;
}

int
strategy::priority_()
{
	if ( ! active_tasklet) throw tasklet_error("not a tasklet");
	return active_tasklet->priority();
}

void
strategy::priority_( int prio)
{
	if ( ! active_tasklet) throw tasklet_error("not a tasklet");
	active_tasklet->priority( prio);
}

void
strategy::yield_()
{
	if ( ! active_tasklet) throw tasklet_error("not a tasklet");
	if ( ! active_tasklet->impl_->attached_strategy() ) throw scheduler_error("no valid scheduler");
	active_tasklet->impl_->attached_strategy()->yield();
}

void
strategy::cancel_()
{
	if ( ! active_tasklet) throw tasklet_error("not a tasklet");
	if ( ! active_tasklet->impl_->attached_strategy() ) throw scheduler_error("no valid scheduler");
	active_tasklet->impl_->attached_strategy()->cancel( * active_tasklet);
}

void
strategy::submit_tasklet_( tasklet f)
{
	if ( ! active_tasklet) throw tasklet_error("not a tasklet");
	if ( ! active_tasklet->impl_->attached_strategy() ) throw scheduler_error("no valid scheduler");
	active_tasklet->impl_->attached_strategy()->add( f);
}

strategy::strategy()
{}

strategy::~strategy()
{}

void
strategy::call( tasklet & f)
{
	BOOST_ASSERT( f);
	BOOST_ASSERT( f.impl_->attached_strategy() );
	BOOST_ASSERT( detail::STATE_RUNNING == f.impl_->state() );
	f.impl_->run();
}

void
strategy::yield( tasklet & f)
{
	BOOST_ASSERT( f);
	BOOST_ASSERT( f.impl_->attached_strategy() );
	BOOST_ASSERT( detail::STATE_RUNNING == f.impl_->state() || detail::STATE_READY == f.impl_->state() );
	f.impl_->yield();
}

void
strategy::attach( tasklet & f)
{ f.impl_->attached_strategy( this); }

void
strategy::detach( tasklet & f)
{ f.impl_->attached_strategy( 0); }

void
strategy::enable_interruption( tasklet & f)
{
	detail::interrupt_type intr( f.impl_->interrupt() );
	// remove disabled flag
	intr &= ~detail::INTERRUPTION_DISABLED;
	// set enabled flag
	intr |= detail::INTERRUPTION_ENABLED;
	f.impl_->interrupt( intr);
}

bool
strategy::interruption_enabled( tasklet const& f)
{ return detail::INTERRUPTION_ENABLED == f.impl_->interrupt(); }

bool
strategy::in_state_not_started( tasklet const& f)
{ return detail::STATE_NOT_STARTED == f.impl_->state(); }

bool
strategy::in_state_ready( tasklet const& f)
{ return detail::STATE_READY == f.impl_->state(); }

bool
strategy::in_state_running( tasklet const& f)
{ return detail::STATE_RUNNING == f.impl_->state(); }

bool
strategy::in_state_wait_for_tasklet( tasklet const& f)
{ return detail::STATE_WAIT_FOR_TASKLET == f.impl_->state(); }

bool
strategy::in_state_wait_for_object( tasklet const& f)
{ return detail::STATE_WAIT_FOR_OBJECT == f.impl_->state(); }

bool
strategy::in_state_terminated( tasklet const& f)
{ return detail::STATE_TERMINATED == f.impl_->state(); }

void
strategy::set_state_ready( tasklet & f)
{ f.impl_->state( detail::STATE_READY); }

void
strategy::set_state_running( tasklet & f)
{ f.impl_->state( detail::STATE_RUNNING); }

void
strategy::set_state_wait_for_tasklet( tasklet & f)
{ f.impl_->state( detail::STATE_WAIT_FOR_TASKLET); }

void
strategy::set_state_wait_for_object( tasklet & f)
{ f.impl_->state( detail::STATE_WAIT_FOR_OBJECT); }

void
strategy::set_state_terminated( tasklet & f)
{ f.impl_->state( detail::STATE_TERMINATED); }

}}
