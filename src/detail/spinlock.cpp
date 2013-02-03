
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_FIBERS_SOURCE

#include <boost/fiber/detail/spinlock.hpp>

#include <boost/assert.hpp>

#include <boost/fiber/detail/scheduler.hpp>
#include <boost/fiber/detail/thread_yield.hpp>

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
        if ( scheduler::instance().active() )
            scheduler::instance().yield();
        else
            thread_yield();
    }
}

void
spinlock::unlock()
{ state_ = UNLOCKED; }

}}}
