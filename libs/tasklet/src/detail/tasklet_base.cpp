
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_TASKLET_SOURCE

#include <boost/tasklet/detail/tasklet_base.hpp>

#include <boost/assert.hpp>

#include <boost/tasklet/exceptions.hpp>
#include <boost/tasklet/utility.hpp>

#include <boost/config/abi_prefix.hpp>

namespace boost {
namespace tasklets {
namespace detail {

BOOST_TASKLET_DECL void trampoline( void * vp)
{
	BOOST_ASSERT( vp);
	detail::tasklet_base * self( static_cast< detail::tasklet_base * >( vp) );
	try
	{ self->exec(); }
	catch ( tasklet_interrupted const&) {}
	catch (...) {}
 	this_tasklet::cancel();
}

void
tasklet_base::run()
{ fib_.run(); }

void
tasklet_base::yield()
{ fib_.yield(); }

int
tasklet_base::priority() const
{ return priority_; }

void
tasklet_base::priority( int prio)
{ priority_ = prio; }

state_type
tasklet_base::state() const
{ return state_; }

void
tasklet_base::state( state_type st)
{ state_ = st; }

interrupt_type &
tasklet_base::interrupt()
{ return interrupt_; }

void
tasklet_base::interrupt( interrupt_type intr)
{ interrupt_ = intr; }

strategy *
tasklet_base::attached_strategy()
{ return st_; }

void
tasklet_base::attached_strategy( strategy * st)
{ st_ = st; }

}}}

#include <boost/config/abi_suffix.hpp>
