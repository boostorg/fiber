
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/fiber/mutex.hpp"

#include <algorithm> // std::find()
#include <mutex> // std::unique_lock

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
mutex::lock_if_unlocked_() {
    if ( mutex_status::unlocked != state_) {
        return false;
    }
    
    state_ = mutex_status::locked;
    BOOST_ASSERT( ! owner_);
    owner_ = this_fiber::get_id();
    return true;
}

mutex::mutex() :
    splk_(),
	state_( mutex_status::unlocked),
    owner_(),
    waiting_() {
}

mutex::~mutex() {
    BOOST_ASSERT( ! owner_);
    BOOST_ASSERT( waiting_.empty() );
}

void
mutex::lock() {
    detail::fiber_handle f( fm_active() );
    for (;;) {
        std::unique_lock< detail::spinlock > lk( splk_);

        if ( lock_if_unlocked_() ) {
            return;
        }

        // store this fiber in order to be notified later
        BOOST_ASSERT( waiting_.end() == std::find( waiting_.begin(), waiting_.end(), f) );
        waiting_.push_back( f);

        // suspend this fiber
        fm_wait( lk);
    }
}

bool
mutex::try_lock() {
    std::unique_lock< detail::spinlock > lk( splk_);

    if ( lock_if_unlocked_() ) {
        return true;
    }

    lk.unlock();
    // let other fiber release the lock
    this_fiber::yield();
    return false;
}

void
mutex::unlock() {
    BOOST_ASSERT( mutex_status::locked == state_);
    BOOST_ASSERT( this_fiber::get_id() == owner_);

    std::unique_lock< detail::spinlock > lk( splk_);
    detail::fiber_handle f;
    if ( ! waiting_.empty() ) {
        f = waiting_.front();
        waiting_.pop_front();
    }
    owner_ = detail::fiber_base::id();
	state_ = mutex_status::unlocked;
    lk.unlock();
    if ( f) {
        f->set_ready();
    }
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
