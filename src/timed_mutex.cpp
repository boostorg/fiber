
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/fiber/timed_mutex.hpp"

#include <algorithm>

#include <boost/assert.hpp>

#include "boost/fiber/fiber_manager.hpp"
#include "boost/fiber/interruption.hpp"
#include "boost/fiber/operations.hpp"

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
    owner_ = this_fiber::get_id();
    return true;
}

timed_mutex::timed_mutex() :
    splk_(),
	state_( mutex_status::unlocked),
    owner_(),
    waiting_() {
}

timed_mutex::~timed_mutex() {
    BOOST_ASSERT( ! owner_);
    BOOST_ASSERT( waiting_.empty() );
}

void
timed_mutex::lock() {
    fiber_context * f( detail::scheduler::instance()->active() );
    BOOST_ASSERT( nullptr != f);
    for (;;) {
        detail::spinlock_lock lk( splk_);

        if ( lock_if_unlocked_() ) {
            return;
        }

        // store this fiber in order to be notified later
        BOOST_ASSERT( waiting_.end() == std::find( waiting_.begin(), waiting_.end(), f) );
        waiting_.push_back( f);

        // suspend this fiber
        detail::scheduler::instance()->wait( lk);
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
    this_fiber::yield();
    return false;
}

bool
timed_mutex::try_lock_until( std::chrono::high_resolution_clock::time_point const& timeout_time) {
    fiber_context * f( detail::scheduler::instance()->active() );
    BOOST_ASSERT( nullptr != f);
    for (;;) {
        detail::spinlock_lock lk( splk_);

        if ( std::chrono::high_resolution_clock::now() > timeout_time) {
            return false;
        }

        if ( lock_if_unlocked_() ) {
            return true;
        }

        // store this fiber in order to be notified later
        BOOST_ASSERT( waiting_.end() == std::find( waiting_.begin(), waiting_.end(), f) );
        waiting_.push_back( f);

        // suspend this fiber until notified or timed-out
        if ( ! detail::scheduler::instance()->wait_until( timeout_time, lk) ) {
            lk.lock();
            std::deque< fiber_context * >::iterator i( std::find( waiting_.begin(), waiting_.end(), f) );
            if ( waiting_.end() != i) {
                // remove fiber from waiting-list
                waiting_.erase( i);
            }
            lk.unlock();
            return false;
        }
    }
}

void
timed_mutex::unlock() {
    BOOST_ASSERT( mutex_status::locked == state_);
    BOOST_ASSERT( this_fiber::get_id() == owner_);

    detail::spinlock_lock lk( splk_);
    fiber_context * f( nullptr);
    if ( ! waiting_.empty() ) {
        f = waiting_.front();
        waiting_.pop_front();
        BOOST_ASSERT( nullptr != f);
    }
    owner_ = fiber_context::id();
    state_ = mutex_status::unlocked;
    lk.unlock();

    if ( nullptr != f) {
        BOOST_ASSERT( ! f->is_terminated() );
        f->set_ready();
    }
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
