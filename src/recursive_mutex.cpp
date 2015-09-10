
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/fiber/recursive_mutex.hpp"

#include <algorithm>

#include <boost/assert.hpp>

#include "boost/fiber/scheduler.hpp"
#include "boost/fiber/interruption.hpp"
#include "boost/fiber/operations.hpp"

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

bool
recursive_mutex::lock_if_unlocked_() {
    if ( mutex_status::unlocked == state_) {
        state_ = mutex_status::locked;
        BOOST_ASSERT( ! owner_);
        owner_ = this_fiber::get_id();
        ++count_;
        return true;
    } else if ( this_fiber::get_id() == owner_) {
        ++count_;
        return true;
    }

    return false;
}

recursive_mutex::recursive_mutex() :
    splk_(),
	state_( mutex_status::unlocked),
    owner_(),
    count_( 0),
    wait_queue_() {
}

recursive_mutex::~recursive_mutex() {
    BOOST_ASSERT( ! owner_);
    BOOST_ASSERT( 0 == count_);
    BOOST_ASSERT( wait_queue_.empty() );
}

void
recursive_mutex::lock() {
    context * f( context::active() );
    BOOST_ASSERT( nullptr != f);
    for (;;) {
        detail::spinlock_lock lk( splk_);

        if ( lock_if_unlocked_() ) {
            return;
        }

        // store this fiber in order to be notified later
        BOOST_ASSERT( ! f->wait_is_linked() );
        wait_queue_.push_back( * f);

        // suspend this fiber
        context::active()->do_wait( lk);
    }
}

bool
recursive_mutex::try_lock() {
    detail::spinlock_lock lk( splk_);

    if ( lock_if_unlocked_() ) {
        return true;
    }

    lk.unlock();

    // let other fiber release the lock
    this_fiber::yield();
    return false;
}

void
recursive_mutex::unlock() {
    BOOST_ASSERT( mutex_status::locked == state_);
    BOOST_ASSERT( this_fiber::get_id() == owner_);

    detail::spinlock_lock lk( splk_);
    context * f( nullptr);
    if ( 0 == --count_) {
        if ( ! wait_queue_.empty() ) {
            f = & wait_queue_.front();
            wait_queue_.pop_front();
            BOOST_ASSERT( nullptr != f);
        }
        owner_ = context::id();
        state_ = mutex_status::unlocked;
        lk.unlock();

        if ( nullptr != f) {
            BOOST_ASSERT( ! f->is_terminated() );
            f->set_ready();
        }
    }
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
