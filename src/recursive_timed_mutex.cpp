
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/fiber/recursive_timed_mutex.hpp"

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
recursive_timed_mutex::lock_if_unlocked_() {
    if ( mutex_status::locked == state_.load( std::memory_order_relaxed) ) {
        if ( context::active() == owner_) {
            ++count_;
            return true;
        } else {
            return false;
        }
    }

    if ( mutex_status::unlocked != state_.exchange( mutex_status::locked, std::memory_order_acquire) ) {
        if ( context::active() == owner_) {
            ++count_;
            return true;
        } else {
            return false;
        }
    }

    BOOST_ASSERT( nullptr == owner_);
    owner_ = context::active();
    ++count_;
    return true;
}

recursive_timed_mutex::recursive_timed_mutex() :
	state_( mutex_status::unlocked),
    owner_( nullptr),
    count_( 0),
    wait_queue_(),
    wait_queue_splk_() {
}

recursive_timed_mutex::~recursive_timed_mutex() {
    BOOST_ASSERT( nullptr == owner_);
    BOOST_ASSERT( 0 == count_);
    BOOST_ASSERT( wait_queue_.empty() );
}

void
recursive_timed_mutex::lock() {
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
recursive_timed_mutex::try_lock() {
    if ( lock_if_unlocked_() ) {
        return true;
    }

    // let other fiber release the lock
    context::active()->yield();
    return false;
}

bool
recursive_timed_mutex::try_lock_until_( std::chrono::steady_clock::time_point const& timeout_time) {
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
            if ( ! ctx->wait_until( timeout_time) ) {
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
recursive_timed_mutex::unlock() {
    BOOST_ASSERT( mutex_status::locked == state_);
    BOOST_ASSERT( context::active() == owner_);

    detail::spinlock_lock lk( wait_queue_splk_);
    context * ctx( nullptr);
    if ( 0 == --count_) {
        if ( ! wait_queue_.empty() ) {
            ctx = & wait_queue_.front();
            wait_queue_.pop_front();
            BOOST_ASSERT( nullptr != ctx);
        }
        lk.unlock();
        owner_ = nullptr;
        state_.store( mutex_status::unlocked, std::memory_order_release);

        if ( nullptr != ctx) {
            context::active()->set_ready( ctx);
        }
    }
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
