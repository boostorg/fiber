
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/fiber/detail/spinlock.hpp"

#include <boost/assert.hpp>

#include "boost/fiber/fiber_context.hpp"
#include "boost/fiber/fiber_manager.hpp"

namespace boost {
namespace fibers {
namespace detail {

atomic_spinlock::atomic_spinlock() noexcept :
    state_( atomic_spinlock_status::unlocked) {
}

void
atomic_spinlock::lock() {
    do {
        // access to CPU's cache
        // first access to state_ -> cache miss
        // sucessive acccess to state_ -> cache hit
        while ( atomic_spinlock_status::locked == state_.load( std::memory_order_relaxed) ) {
            // busy-wait
            fiber_context::active()->do_yield();
        }
        // state_ was released by other fiber
        // cached copies are invalidated -> cache miss
        // test-and-set signaled over the bus 
    }
    while ( atomic_spinlock_status::unlocked != state_.exchange( atomic_spinlock_status::locked, std::memory_order_acquire) );
}

void
atomic_spinlock::unlock() noexcept {
    BOOST_ASSERT( atomic_spinlock_status::locked == state_);
    state_ = atomic_spinlock_status::unlocked;
}

}}}
