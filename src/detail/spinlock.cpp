
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/fiber/detail/spinlock.hpp>

#include <boost/assert.hpp>

#include <boost/fiber/fiber_manager.hpp>

namespace boost {
namespace fibers {
namespace detail {

spinlock::spinlock() noexcept :
    state_( spinlock_status::unlocked) {
}

void
spinlock::lock() {
    do {
        // access to CPU's cache
        // first access to state_ -> cache miss
        // sucessive acccess to state_ -> cache hit
        while ( spinlock_status::locked == state_.load( std::memory_order_relaxed) ) {
            // busy-wait
            fm_yield();
        }
        // state_ was released by other fiber
        // cached copies are invalidated -> cache miss
        // test-and-set signaled over the bus 
    }
    while ( spinlock_status::unlocked != state_.exchange( spinlock_status::locked, std::memory_order_acquire) );
}

void
spinlock::unlock() noexcept {
    BOOST_ASSERT( spinlock_status::locked == state_);
    state_ = spinlock_status::unlocked;
}

}}}
