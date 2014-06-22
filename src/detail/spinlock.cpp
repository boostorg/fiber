
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/fiber/detail/spinlock.hpp>

#include <boost/assert.hpp>
#include <boost/thread/thread.hpp>

#include <boost/fiber/fiber_manager.hpp>

namespace boost {
namespace fibers {
namespace detail {

spinlock::spinlock() :
    state_( UNLOCKED)
{}

void
spinlock::lock()
{
    for (;;)
    {
        // access to CPU's cache
        // first access to state_ -> cache miss
        // sucessive acccess to state_ > cache hit
        while ( LOCKED == state_)
        {
            // busy-wait
            if ( 0 != fm_active() )
                fm_yield();
           else
                this_thread::yield();
        }
        // state_ was released by other
        // cached copies are invalidated -> cache miss
        // test-and-set over the bus 
        if ( UNLOCKED == state_.exchange( LOCKED) )
            return;
    }
}

void
spinlock::unlock()
{
    BOOST_ASSERT( LOCKED == state_);

    state_ = UNLOCKED;
}

}}}
