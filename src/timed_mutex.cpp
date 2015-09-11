
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
    if ( mutex_status::unlocked != state_) {
        return false;
    }
    
    state_ = mutex_status::locked;
    BOOST_ASSERT( ! owner_);
    owner_ = context::active()->get_id();
    return true;
}

timed_mutex::timed_mutex() :
    splk_(),
	state_( mutex_status::unlocked),
    owner_(),
    wait_queue_() {
}

timed_mutex::~timed_mutex() {
    BOOST_ASSERT( ! owner_);
    BOOST_ASSERT( wait_queue_.empty() );
}

void
timed_mutex::lock() {
    context * f( context::active() );
    BOOST_ASSERT( nullptr != f);
    for (;;) {
        detail::spinlock_lock lk( splk_);

        if ( lock_if_unlocked_() ) {
            return;
        }

        try {
            // store this fiber in order to be notified later
            BOOST_ASSERT( ! f->wait_is_linked() );
            f->set_waiting();
            wait_queue_.push_back( * f);
            lk.unlock();

            // check if fiber was interrupted
            this_fiber::interruption_point();
            // suspend this fiber
            f->do_schedule();
        } catch (...) {
            detail::spinlock_lock lk( splk_);
            f->wait_unlink();
            throw;
        }
    }
}

bool
timed_mutex::try_lock() {
    detail::spinlock_lock lk( splk_);

    if ( lock_if_unlocked_() ) {
        return true;
    }

    lk.unlock();

    // let other fiber release the lock
    context::active()->do_yield();
    return false;
}

bool
timed_mutex::try_lock_until_( std::chrono::steady_clock::time_point const& timeout_time) {
    context * f( context::active() );
    BOOST_ASSERT( nullptr != f);
    for (;;) {
        detail::spinlock_lock lk( splk_);

        if ( std::chrono::steady_clock::now() > timeout_time) {
            return false;
        }

        if ( lock_if_unlocked_() ) {
            return true;
        }

        try {
            // store this fiber in order to be notified later
            BOOST_ASSERT( ! f->wait_is_linked() );
            f->set_waiting();
            wait_queue_.push_back( * f);
            lk.unlock();

            // check if fiber was interrupted
            this_fiber::interruption_point();
            // suspend this fiber until notified or timed-out
            if ( ! context::active()->do_wait_until( timeout_time) ) {
                lk.lock();
                f->wait_unlink();
                lk.unlock();
                return false;
            }
        } catch (...) {
            detail::spinlock_lock lk( splk_);
            f->wait_unlink();
            throw;
        }
    }
}

void
timed_mutex::unlock() {
    BOOST_ASSERT( mutex_status::locked == state_);
    BOOST_ASSERT( context::active()->get_id() == owner_);

    detail::spinlock_lock lk( splk_);
    context * f( nullptr);
    if ( ! wait_queue_.empty() ) {
        f = & wait_queue_.front();
        wait_queue_.pop_front();
        BOOST_ASSERT( nullptr != f);
    }
    owner_ = context::id();
    state_ = mutex_status::unlocked;
    lk.unlock();

    if ( nullptr != f) {
        context::active()->do_signal( f);
    }
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
