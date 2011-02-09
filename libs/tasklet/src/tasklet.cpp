
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_TASKLET_SOURCE

#include <boost/tasklet/tasklet.hpp>

#include <boost/assert.hpp>

#include <boost/tasklet/detail/interrupt_flags.hpp>
#include <boost/tasklet/detail/state_flags.hpp>
#include <boost/tasklet/exceptions.hpp>
#include <boost/tasklet/strategy.hpp>
#include <boost/tasklet/utility.hpp>

#include <boost/config/abi_prefix.hpp>

namespace boost {
namespace tasklets {

std::size_t
tasklet::default_stacksize = 65536;

tasklet::tasklet() :
	impl_()
{}

tasklet::tasklet( tasklet const& other) :
	impl_( other.impl_)
{}

tasklet &
tasklet::operator=( BOOST_COPY_ASSIGN_REF( tasklet) other)
{
	if ( this == & other) return * this;
	impl_ = other.impl_;
	return * this;
}

tasklet::tasklet( BOOST_RV_REF( tasklet) other)
{
	impl_ = other.impl_;
	other.impl_.reset();
}

tasklet &
tasklet::operator=( BOOST_RV_REF( tasklet) other)
{
	tasklet tmp( other);
	swap( tmp);
	return * this;
}

tasklet::operator unspecified_bool_type() const
{ return impl_; }

bool
tasklet::operator!() const
{ return ! impl_; }

bool
tasklet::operator==( tasklet const& other) const
{ return get_id() == other.get_id(); }

bool
tasklet::operator!=( tasklet const& other) const
{ return !( get_id() == other.get_id() ); }

void
tasklet::swap( tasklet & other)
{ impl_.swap( other.impl_); }

tasklet::id
tasklet::get_id() const
{ return tasklet::id( impl_); }

bool
tasklet::is_alive() const
{
	if ( ! impl_) throw tasklet_moved();
	return ( impl_->state() & IS_ALIVE_BIT_MASK) != 0;
}

int
tasklet::priority() const
{
	if ( ! impl_) throw tasklet_moved();
	return impl_->priority();
}

void
tasklet::priority( int prio)
{
	if ( ! impl_) throw tasklet_moved();
	impl_->priority( prio);
	if ( is_alive() )
		impl_->attached_strategy()->reschedule( * this);
}

void
tasklet::interrupt()
{
	if ( ! impl_) throw tasklet_moved();
	impl_->attached_strategy()->interrupt( * this);
}

bool
tasklet::interruption_requested() const
{
	if ( ! impl_) throw tasklet_moved();
	return ( impl_->interrupt() & detail::INTERRUPTION_ENABLED) != 0;
}

void
tasklet::cancel()
{
	if ( ! impl_) throw tasklet_moved();
	impl_->attached_strategy()->cancel( * this);
}

void
tasklet::join()
{
	if ( ! impl_) throw tasklet_moved();
	impl_->attached_strategy()->join( * this);
}

}}

#include <boost/config/abi_suffix.hpp>
