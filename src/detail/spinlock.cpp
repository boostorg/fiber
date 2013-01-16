
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_FIBERS_SOURCE

#include <boost/fiber/detail/spinlock.hpp>

#include <boost/assert.hpp>
#include <boost/fiber/operations.hpp>

namespace boost {
namespace fibers {
namespace detail {

spinlock::spinlock() :
	state_( UNLOCKED)
{}

void
spinlock::lock()
{
    while ( LOCKED == state_.exchange( LOCKED, memory_order_seq_cst) )
    {
        // busy-wait
		// BOOST_ASSERT( this_fiber::is_fiberized() );
		if ( this_fiber::is_fiberized() )
		    this_fiber::yield();
    }
}

void
spinlock::unlock()
{ state_ = UNLOCKED; }

}}}
