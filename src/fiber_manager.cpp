
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/fiber/fiber_manager.hpp"

#include <cmath>
#include <thread> // std::this_thread::sleep_until()
#include <utility>

#include <boost/assert.hpp>

#include "boost/fiber/algorithm.hpp"
#include "boost/fiber/exceptions.hpp"
#include "boost/fiber/fiber_context.hpp"
#include "boost/fiber/interruption.hpp"
#include "boost/fiber/round_robin.hpp"

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

fiber_manager::fiber_manager() noexcept :
    sched_algo_( new round_robin() ),
    wqueue_(),
    tqueue_(),
    wait_interval_( std::chrono::milliseconds( 10) ) {
}

fiber_manager::~fiber_manager() noexcept {
    // FIXME: test for main-fiber
    for (;;) {
        // NOTE: at this stage the fibers in the waiting-queue
        //       can only be detached fibers
        //       interrupt all waiting fibers (except main-fiber)
        wqueue_.interrupt_all();
        // move all fibers which are ready (state_ready)
        // from waiting-queue to the runnable-queue
        wqueue_.move_to( sched_algo_.get() );
        // pop new fiber from ready-queue
        fiber_context * f( sched_algo_->pick_next() );
        if ( f) {
            BOOST_ASSERT_MSG( f->is_ready(), "fiber with invalid state in ready-queue");
            // set fiber_manager
            f->manager( this);
            // add active-fiber to joinig-list of f
            // fiber::join() should not fail because fiber f is in state_ready
            // so main-fiber should be in the waiting-queue of fiber f
            f->join( fiber_context::active() );
            // set main-fiber to state_waiting
            fiber_context::active()->set_waiting();
            // push main-fiber to waiting-queue
            wqueue_.push( fiber_context::active() );
            // resume fiber f
            resume_( f);
        } else if ( wqueue_.empty() ) {
            // ready- and waiting-queue are empty so we can quit
            break;
        }
    }
    BOOST_ASSERT( wqueue_.empty() );
    // FIXME: test for main-fiber
}

void
fiber_manager::resume_( fiber_context * f) {
    BOOST_ASSERT( nullptr != f);
    BOOST_ASSERT( f->is_ready() );
    // set fiber to state running
    f->set_running();
    // fiber next-to-run is same as current active-fiber
    // this might happen in context of this_fiber::yield() 
    if ( f == fiber_context::active() ) {
        return;
    }
    // pass new fiber the fiber_manager of the current active fiber
    // this might be necessary if the new fiber was miggrated from
    // another thread
    f->manager( fiber_context::active()->manager() );
    // assign new fiber to active-fiber
    fiber_context * old( fiber_context::active( f) );
    // push terminated fibers to terminated-queue
    if ( old->is_terminated() ) {
        tqueue_.push( old);
    }
    // resume active-fiber == start or yield to
    fiber_context::active()->resume();
}

void
fiber_manager::spawn( fiber_context * f) {
    BOOST_ASSERT( nullptr != f);
    BOOST_ASSERT( f->is_ready() );
    BOOST_ASSERT( f != fiber_context::active() );
    // add new fiber to the scheduler
    sched_algo_->awakened( f);
}

void
fiber_manager::run() {
    for (;;) {
        // destroy terminated fibers from terminated-queue
        tqueue_.clear();
        // move all fibers which are ready (state_ready)
        // from waiting-queue to the runnable-queue
        wqueue_.move_to( sched_algo_.get() );
        // pop new fiber from ready-queue
        fiber_context * f( sched_algo_->pick_next() );
        if ( f) {
            BOOST_ASSERT_MSG( f->is_ready(), "fiber with invalid state in ready-queue");
            // set fiber_manager
            f->manager( this);
            // resume fiber f
            resume_( f);
            return;
        } else {
            // no fibers ready to run; the thread should sleep
            std::this_thread::sleep_until(
                std::chrono::steady_clock::now() + wait_interval_);
        }
    }
}

void
fiber_manager::wait( detail::spinlock_lock & lk) {
    wait_until(
        std::chrono::steady_clock::time_point(
            (std::chrono::steady_clock::duration::max)() ),
        lk);
}

bool
fiber_manager::wait_until_( std::chrono::steady_clock::time_point const& timeout_time,
                            detail::spinlock_lock & lk) {
    BOOST_ASSERT( fiber_context::active()->is_running() );
    // set active-fiber to state_waiting
    fiber_context::active()->set_waiting();
    // release lock
    lk.unlock();
    // push active-fiber to waiting-queue
    fiber_context::active()->time_point( timeout_time);
    wqueue_.push( fiber_context::active() );
    // switch to another fiber
    run();
    // fiber has been resumed
    // check if fiber was interrupted
    this_fiber::interruption_point();
    // check if timeout has reached
    return std::chrono::steady_clock::now() < timeout_time;
}

void
fiber_manager::yield() {
    BOOST_ASSERT( fiber_context::active()->is_running() );
    // set active-fiber to state_waiting
    fiber_context::active()->set_ready();
    // push active-fiber to wqueue_
    wqueue_.push( fiber_context::active() );
    // switch to another fiber
    run();
    // fiber has been resumed
    // NOTE: do not check if fiber was interrupted
    //       yield() should not be an interruption point
}

void
fiber_manager::join( fiber_context * f) {
    BOOST_ASSERT( nullptr != f);
    BOOST_ASSERT( f != fiber_context::active() );
    // set active-fiber to state_waiting
    fiber_context::active()->set_waiting();
    // push active-fiber to waiting-queue
    wqueue_.push( fiber_context::active() );
    // add active-fiber to joinig-list of f
    if ( ! f->join( fiber_context::active() ) ) {
        // f must be already terminated therefore we set
        // active-fiber to state_ready
        // FIXME: better state_running and no suspend
        fiber_context::active()->set_ready();
    }
    // switch to another fiber
    run();
    // fiber has been resumed
    // check if fiber was interrupted
    this_fiber::interruption_point();
    // check that fiber f has terminated
    BOOST_ASSERT( f->is_terminated() );
}

void
fiber_manager::set_sched_algo( std::unique_ptr< sched_algorithm > algo) {
    sched_algo_ = std::move( algo);
}

void
fiber_manager::wait_interval( std::chrono::steady_clock::duration const& wait_interval) noexcept {
    wait_interval_ = wait_interval;
}

std::chrono::steady_clock::duration
fiber_manager::wait_interval() noexcept {
    return wait_interval_;
}

std::size_t
fiber_manager::ready_fibers() const noexcept {
    return sched_algo_->ready_fibers();
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
