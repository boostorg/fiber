
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/fiber/scheduler.hpp"

#include <cmath>
#include <thread> // std::this_thread::sleep_until()
#include <utility>

#include <boost/assert.hpp>

#include "boost/fiber/algorithm.hpp"
#include "boost/fiber/exceptions.hpp"
#include "boost/fiber/context.hpp"
#include "boost/fiber/interruption.hpp"
#include "boost/fiber/round_robin.hpp"

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

scheduler::scheduler( context * main_context) noexcept :
    sched_algo_( new round_robin() ),
    main_context_( main_context),
    wqueue_(),
    tqueue_(),
    wait_interval_( std::chrono::milliseconds( 10) ) {
}

scheduler::~scheduler() noexcept {
    BOOST_ASSERT( context::active() == main_context_);
    for (;;) {
        // NOTE: at this stage the fibers in the waiting-queue
        //       can only be detached fibers
        //       interrupt all waiting fibers (except main-fiber)
        wqueue_.interrupt_all();
        // move all fibers which are ready (state_ready)
        // from waiting-queue to the runnable-queue
        wqueue_.move_to( sched_algo_.get() );
        // pop new fiber from ready-queue
        context * f( sched_algo_->pick_next() );
        if ( f) {
            BOOST_ASSERT_MSG( f->is_ready(), "fiber with invalid state in ready-queue");
            // set scheduler
            f->set_scheduler( this);
            // add active-fiber to joinig-list of f
            // fiber::join() should not fail because fiber f is in state_ready
            // so main-fiber should be in the waiting-queue of fiber f
            f->join( main_context_);
            // set main-fiber to state_waiting
            main_context_->set_waiting();
            // push main-fiber to waiting-queue
            wqueue_.push( main_context_);
            // resume fiber f
            resume_( main_context_, f);
        } else if ( wqueue_.empty() ) {
            // ready- and waiting-queue are empty so we can quit
            break;
        }
    }
    BOOST_ASSERT( wqueue_.empty() );
    BOOST_ASSERT( context::active() == main_context_);
}

void
scheduler::resume_( context * af, context * f) {
    BOOST_ASSERT( nullptr != af);
    BOOST_ASSERT( nullptr != f);
    BOOST_ASSERT( f->is_ready() );
    // set fiber to state running
    f->set_running();
    // fiber next-to-run is same as current active-fiber
    // this might happen in context of this_fiber::yield() 
    if ( f == af) {
        return;
    }
    // pass new fiber the scheduler of the current active fiber
    // this might be necessary if the new fiber was miggrated from
    // another thread
    f->set_scheduler( af->get_scheduler() );
    // assign new fiber to active-fiber
    context::active( f);
    // push terminated fibers to terminated-queue
    if ( af->is_terminated() ) {
        tqueue_.push( af);
    }
    // resume active-fiber == f
    f->resume();
}

bool
scheduler::wait_until_( context * af,
                        std::chrono::steady_clock::time_point const& timeout_time,
                        detail::spinlock_lock & lk) {
    BOOST_ASSERT( nullptr != af);
    BOOST_ASSERT( context::active() == af);
    BOOST_ASSERT( af->is_running() );
    // set active-fiber to state_waiting
    af->set_waiting();
    // release lock
    lk.unlock();
    // push active-fiber to waiting-queue
    af->time_point( timeout_time);
    wqueue_.push( af);
    // switch to another fiber
    run( af);
    // fiber has been resumed
    // check if fiber was interrupted
    this_fiber::interruption_point();
    // check if timeout has reached
    return std::chrono::steady_clock::now() < timeout_time;
}

void
scheduler::spawn( context * f) {
    BOOST_ASSERT( nullptr != f);
    BOOST_ASSERT( f->is_ready() );
    BOOST_ASSERT( f != context::active() );
    // add new fiber to the scheduler
    sched_algo_->awakened( f);
}

void
scheduler::run( context * af) {
    BOOST_ASSERT( nullptr != af);
    BOOST_ASSERT( context::active() == af);
    for (;;) {
        // move all fibers which are ready (state_ready)
        // from waiting-queue to the runnable-queue
        wqueue_.move_to( sched_algo_.get() );
        // pop new fiber from ready-queue
        context * f( sched_algo_->pick_next() );
        if ( f) {
            BOOST_ASSERT_MSG( f->is_ready(), "fiber with invalid state in ready-queue");
            // resume fiber f
            resume_( af, f);
            // destroy terminated fibers from terminated-queue
            tqueue_.clear();
            return;
        } else {
            // no fibers ready to run; the thread should sleep
            std::this_thread::sleep_until(
                std::chrono::steady_clock::now() + wait_interval_);
        }
    }
}

void
scheduler::wait( context * af, detail::spinlock_lock & lk) {
    wait_until(
        af,
        std::chrono::steady_clock::time_point(
            (std::chrono::steady_clock::duration::max)() ),
        lk);
}

void
scheduler::yield( context * af) {
    BOOST_ASSERT( nullptr != af);
    BOOST_ASSERT( context::active() == af);
    BOOST_ASSERT( af->is_running() );
    // set active-fiber to state_waiting
    af->set_ready();
    // push active-fiber to wqueue_
    wqueue_.push( af);
    // switch to another fiber
    run( af);
    // fiber has been resumed
    // NOTE: do not check if fiber was interrupted
    //       yield() should not be an interruption point
}

void
scheduler::join( context * af, context * f) {
    BOOST_ASSERT( nullptr != af);
    BOOST_ASSERT( context::active() == af);
    BOOST_ASSERT( af->is_running() );
    BOOST_ASSERT( nullptr != f);
    BOOST_ASSERT( f != af);
    // set active-fiber to state_waiting
    af->set_waiting();
    // push active-fiber to waiting-queue
    wqueue_.push( af);
    // add active-fiber to joinig-list of f
    if ( ! f->join( af) ) {
        // f must be already terminated therefore we set
        // active-fiber to state_ready
        // FIXME: better state_running and no suspend
        af->set_ready();
    }
    // switch to another fiber
    run( af);
    // fiber has been resumed
    // check if fiber was interrupted
    this_fiber::interruption_point();
    // check that fiber f has terminated
    BOOST_ASSERT( f->is_terminated() );
}

void
scheduler::set_sched_algo( std::unique_ptr< sched_algorithm > algo) {
    sched_algo_ = std::move( algo);
}

void
scheduler::wait_interval( std::chrono::steady_clock::duration const& wait_interval) noexcept {
    wait_interval_ = wait_interval;
}

std::chrono::steady_clock::duration
scheduler::wait_interval() noexcept {
    return wait_interval_;
}

std::size_t
scheduler::ready_fibers() const noexcept {
    return sched_algo_->ready_fibers();
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
