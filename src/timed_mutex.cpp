
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/fiber/timed_mutex.hpp"

#include <algorithm>

#include <boost/assert.hpp>

#include "boost/fiber/scheduler.hpp"
#include "boost/fiber/interruption.hpp"

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

bool
timed_mutex::lock_if_unlocked_() {
    if ( mutex_status::locked == state_.load( std::memory_order_relaxed) ) {
        return false;
    }
    if ( mutex_status::unlocked != state_.exchange( mutex_status::locked, std::memory_order_acquire) ) {
        return false;
    }
    BOOST_ASSERT( nullptr == owner_.load());
    owner_ = context::active();
    return true;
}

timed_mutex::timed_mutex() :
	state_( mutex_status::unlocked),
    owner_( nullptr),
    wait_queue_(),
    wait_queue_splk_() {
}

timed_mutex::~timed_mutex() {
    BOOST_ASSERT( nullptr == owner_.load());
    BOOST_ASSERT( wait_queue_.empty() );
}

void
timed_mutex::lock() {
    context * ctx = context::active();
    for (;;) {
        try {
            if ( lock_if_unlocked_() ) {
                return;
            }
            // store this fiber in order to be notified later
            detail::spinlock_lock lk( wait_queue_splk_);
            BOOST_ASSERT( ! ctx->wait_is_linked() );
            wait_queue_.push_back( * ctx);
            lk.unlock();
            // suspend this fiber
            ctx->suspend();
            // remove fiber from wait-queue 
            lk.lock();
            ctx->wait_unlink();
        } catch (...) {
            // remove fiber from wait-queue 
            detail::spinlock_lock lk( wait_queue_splk_);
            ctx->wait_unlink();
            throw;
        }
    }
}

bool
timed_mutex::try_lock() {
    if ( lock_if_unlocked_() ) {
        return true;
    }
    // let other fiber release the lock
    context::active()->yield();
    return false;
}

bool
timed_mutex::try_lock_until_( std::chrono::steady_clock::time_point const& timeout_time) {
    context * ctx = context::active();
    for (;;) {
        try {
            if ( std::chrono::steady_clock::now() > timeout_time) {
                return false;
            }
            if ( lock_if_unlocked_() ) {
                return true;
            }
            // store this fiber in order to be notified later
            detail::spinlock_lock lk( wait_queue_splk_);
            BOOST_ASSERT( ! ctx->wait_is_linked() );
            wait_queue_.push_back( * ctx);
            lk.unlock();
            // suspend this fiber until notified or timed-out
            if ( ! context::active()->wait_until( timeout_time) ) {
                // remove fiber from wait-queue 
                lk.lock();
                ctx->wait_unlink();
                return false;
            }
            // remove fiber from wait-queue 
            lk.lock();
            ctx->wait_unlink();
        } catch (...) {
            // remove fiber from wait-queue 
            detail::spinlock_lock lk( wait_queue_splk_);
            ctx->wait_unlink();
            throw;
        }
    }
}

void
timed_mutex::unlock() {
    BOOST_ASSERT( mutex_status::locked == state_);
    BOOST_ASSERT( context::active() == owner_);
    detail::spinlock_lock lk( wait_queue_splk_);
    context * ctx( nullptr);
    if ( ! wait_queue_.empty() ) {
        ctx = & wait_queue_.front();
        wait_queue_.pop_front();
        BOOST_ASSERT( nullptr != ctx);
    }
    lk.unlock();
    owner_ = nullptr;
    state_.store( mutex_status::unlocked, std::memory_order_relaxed);
    if ( nullptr != ctx) {
        context::active()->set_ready( ctx);
    }
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
