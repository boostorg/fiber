
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/fiber/detail/spinlock.hpp>

#include <boost/assert.hpp>
#include <boost/thread/thread.hpp>

#include <boost/fiber/detail/scheduler.hpp>

namespace boost {
namespace fibers {
namespace detail {

spinlock::spinlock() :
    state_( UNLOCKED)
{}

void
spinlock::lock()
{
    while ( LOCKED == state_.exchange( LOCKED) )
    {
        // busy-wait
        if ( scheduler::instance()->active() )
            scheduler::instance()->yield();
        else
            this_thread::yield();
    }
}

void
spinlock::unlock()
{ state_ = UNLOCKED; }

}}}
