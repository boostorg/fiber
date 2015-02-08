
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/fiber/fiber_manager.hpp>

#include <cmath>
#include <thread> // std::this_thread::sleep_until()

#include <boost/assert.hpp>

#include "boost/fiber/algorithm.hpp"
#include "boost/fiber/fiber_context.hpp"
#include "boost/fiber/detail/scheduler.hpp"
#include "boost/fiber/exceptions.hpp"
#include "boost/fiber/round_robin.hpp"

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

sched_algorithm * default_algorithm() {
    static thread_local round_robin rr;
    return & rr;
}

fiber_manager::fiber_manager() noexcept :
    sched_algo_( default_algorithm() ),
    active_fiber_( fiber_context::main_fiber() ),
    wqueue_(),
    tqueue_(),
    preserve_fpu_( false),
    wait_interval_( std::chrono::milliseconds( 10) ) {
}

fiber_manager::~fiber_manager() noexcept {
    // fibers will be destroyed (stack-unwinding)
    // if last reference goes out-of-scope
    // therefore destructing wqueue_ && rqueue_
    // will destroy the fibers in this scheduler
    // if not referenced on other places
    while ( ! wqueue_.empty() ) {
        run();
    }
    active_fiber_ = nullptr;
}

void
fiber_manager::resume_( fiber_context * f) {
    BOOST_ASSERT( nullptr != f);
    BOOST_ASSERT( f->is_ready() );

    // set fiber to state running
    f->set_running();

    // fiber next-to-run is same as current active fiber
    // this might happen in context of this_fiber::yield() 
    if ( f == active_fiber_) {
        return;
    }

    // assign new fiber to active-fiber
    std::swap( active_fiber_, f);
    // push terminated fibers to tqueue_
    if ( f->is_terminated() ) {
        tqueue_.push( f);
    }

    // resume active-fiber == start or yield to
    active_fiber_->resume();
}

std::chrono::high_resolution_clock::time_point
fiber_manager::next_wakeup() {
    if ( wqueue_.empty() ) {
        return std::chrono::high_resolution_clock::now() + wait_interval_;
    } else {
        //FIXME: search for the closest time_point to now() in waiting-queue
        std::chrono::high_resolution_clock::time_point wakeup( wqueue_.top()->time_point() );
        if ( (std::chrono::high_resolution_clock::time_point::max)() == wakeup) {
            return std::chrono::high_resolution_clock::now() + wait_interval_;
        }
        return wakeup;
    }
}

void
fiber_manager::spawn( fiber_context * f) {
    BOOST_ASSERT( nullptr != f);
    BOOST_ASSERT( f->is_ready() );
    BOOST_ASSERT( f != active_fiber_);

    sched_algo_->awakened( f);
}

void
fiber_manager::run() {
    for (;;) {
        // move all fibers witch are ready (state_ready)
        // from waiting-queue to the runnable-queue
        wqueue_.move_to( sched_algo_);

        // pop new fiber from ready-queue
        fiber_context * f( sched_algo_->pick_next() );
        if ( f) {
            BOOST_ASSERT_MSG( f->is_ready(), "fiber with invalid state in ready-queue");

            // destroy terminated fibers from tqueue_
            while ( ! tqueue_.empty() ) {
                fiber_context * f_( tqueue_.pop() );
                BOOST_ASSERT( nullptr != f_);
                BOOST_ASSERT( f_->is_terminated() );
                intrusive_ptr_release( f_);
            }

            // resume fiber f
            resume_( f);

            // destroy terminated fibers from tqueue_
            while ( ! tqueue_.empty() ) {
                fiber_context * f_( tqueue_.pop() );
                BOOST_ASSERT( nullptr != f_);
                BOOST_ASSERT( f_->is_terminated() );
                intrusive_ptr_release( f_);
            }

            return;
        } else {
            // no fibers ready to run; the thread should sleep
            // until earliest fiber is scheduled to run
            std::chrono::high_resolution_clock::time_point wakeup( next_wakeup() );
            std::this_thread::sleep_until( wakeup);
        }
    }
}

void
fiber_manager::wait( std::unique_lock< detail::spinlock > & lk) {
    wait_until(
        std::chrono::high_resolution_clock::time_point( (std::chrono::high_resolution_clock::duration::max)() ),
        lk);
}

bool
fiber_manager::wait_until( std::chrono::high_resolution_clock::time_point const& timeout_time,
                    std::unique_lock< detail::spinlock > & lk) {
    BOOST_ASSERT( active_fiber_->is_running() );

    // set active-fiber to state_waiting
    active_fiber_->set_waiting();
    // release lock
    lk.unlock();
    // push active-fiber to wqueue_
    active_fiber_->time_point( timeout_time);
    wqueue_.push( active_fiber_);
    // switch to another fiber
    run();
    // fiber is resumed

    return std::chrono::high_resolution_clock::now() < timeout_time;
}

void
fiber_manager::yield() {
    BOOST_ASSERT( active_fiber_->is_running() );

    // set active-fiber to state_waiting
    active_fiber_->set_ready();
    // push active-fiber to wqueue_
    wqueue_.push( active_fiber_);
    // switch to another fiber
    run();
    // fiber is resumed
}

void
fiber_manager::join( fiber_context * f) {
    BOOST_ASSERT( nullptr != f);
    BOOST_ASSERT( f != active_fiber_);

    // set active-fiber to state_waiting
    active_fiber_->set_waiting();
    // push active-fiber to wqueue_
    wqueue_.push( active_fiber_);
    // add active-fiber to joinig-list of f
    if ( ! f->join( active_fiber_) ) {
        // f must be already terminated therefore we set
        // active-fiber to state_ready
        // FIXME: better state_running and no suspend
        active_fiber_->set_ready();
    }
    // switch to another fiber
    run();
    // fiber is resumed

    BOOST_ASSERT( f->is_terminated() );
}

fiber_context *
fiber_manager::active() noexcept {
    return active_fiber_;
}

void
fiber_manager::set_sched_algo( sched_algorithm * algo) {
    sched_algo_ = algo;
}

void
fiber_manager::wait_interval( std::chrono::high_resolution_clock::duration const& wait_interval) noexcept {
    wait_interval_ = wait_interval;
}

std::chrono::high_resolution_clock::duration
fiber_manager::wait_interval() noexcept {
    return wait_interval_;
}

bool
fiber_manager::preserve_fpu() {
    return preserve_fpu_;
}

void
fiber_manager::preserve_fpu( bool preserve) {
    preserve_fpu_ = preserve;
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
