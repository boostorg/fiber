
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/fiber/fiber_manager.hpp>

#include <cmath>
#include <thread> // std::this_thread::sleep_until()

#include <boost/assert.hpp>

#include "boost/fiber/algorithm.hpp"
#include "boost/fiber/detail/fiber_base.hpp"
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
    sched_algo( default_algorithm() ),
    active_fiber( detail::fiber_base::main_fiber() ),
    wqueue(),
    preserve_fpu( false),
    wait_interval( std::chrono::milliseconds( 10) ) {
}

fiber_manager::~fiber_manager() noexcept {
    // fibers will be destroyed (stack-unwinding)
    // if last reference goes out-of-scope
    // therefore destructing fm->wqueue && rqueue_
    // will destroy the fibers in this scheduler
    // if not referenced on other places
    active_fiber.reset();
}

void fm_resume_( detail::fiber_handle f) {
    fiber_manager * fm = detail::scheduler::instance();

    BOOST_ASSERT( nullptr != fm);
    BOOST_ASSERT( f);
    BOOST_ASSERT( f->is_ready() );

    // set fiber to state running
    f->set_running();

    // fiber next-to-run is same as current active fiber
    // this might happen in context of this_fiber::yield() 
    if ( f == fm->active_fiber) {
        return;
    }

    // assign new fiber to active-fiber
    fm->active_fiber = f;

    // resume active-fiber == start or yield to
    fm->active_fiber->resume();
}

std::chrono::high_resolution_clock::time_point fm_next_wakeup() {
    fiber_manager * fm = detail::scheduler::instance();

    BOOST_ASSERT( nullptr != fm);

    if ( fm->wqueue.empty() ) {
        return std::chrono::high_resolution_clock::now() + fm->wait_interval;
    } else {
        //FIXME: search for the closest time_point to now() in waiting-queue
        std::chrono::high_resolution_clock::time_point wakeup( fm->wqueue.top()->time_point() );
        if ( (std::chrono::high_resolution_clock::time_point::max)() == wakeup) {
            return std::chrono::high_resolution_clock::now() + fm->wait_interval;
        }
        return wakeup;
    }
}

void fm_spawn( detail::fiber_handle f) {
    fiber_manager * fm = detail::scheduler::instance();

    BOOST_ASSERT( nullptr != fm);
    BOOST_ASSERT( f);
    BOOST_ASSERT( f->is_ready() );
    BOOST_ASSERT( f != fm->active_fiber);

    fm->sched_algo->awakened( f);
}

void fm_run() {
    fiber_manager * fm = detail::scheduler::instance();

    BOOST_ASSERT( nullptr != fm);

    for (;;) {
        // move all fibers witch are ready (state_ready)
        // from waiting-queue to the runnable-queue
        fm->wqueue.move_to( fm->sched_algo);

        // pop new fiber from ready-queue
        detail::fiber_handle f( fm->sched_algo->pick_next() );
        if ( f) {
            BOOST_ASSERT_MSG( f->is_ready(), "fiber with invalid state in ready-queue");

            // resume fiber f
            fm_resume_( f);

            return;
        } else {
            // no fibers ready to run; the thread should sleep
            // until earliest fiber is scheduled to run
            std::chrono::high_resolution_clock::time_point wakeup( fm_next_wakeup() );
            std::this_thread::sleep_until( wakeup);
        }
    }
}

void fm_wait( std::unique_lock< detail::spinlock > & lk) {
    fm_wait_until(
        std::chrono::high_resolution_clock::time_point( (std::chrono::high_resolution_clock::duration::max)() ),
        lk);
}

bool fm_wait_until( std::chrono::high_resolution_clock::time_point const& timeout_time,
                    std::unique_lock< detail::spinlock > & lk) {
    fiber_manager * fm = detail::scheduler::instance();

    BOOST_ASSERT( nullptr != fm);
    BOOST_ASSERT( fm->active_fiber);
    BOOST_ASSERT( fm->active_fiber->is_running() );

    std::chrono::high_resolution_clock::time_point start( std::chrono::high_resolution_clock::now() );

    // set active-fiber to state_waiting
    fm->active_fiber->set_waiting();
    // release lock
    lk.unlock();
    // push active-fiber to fm->wqueue
    fm->active_fiber->time_point( timeout_time);
    fm->wqueue.push( fm->active_fiber);
    // switch to another fiber
    fm_run();
    // fiber is resumed

    return std::chrono::high_resolution_clock::now() < timeout_time;
}

void fm_yield() {
    fiber_manager * fm = detail::scheduler::instance();

    BOOST_ASSERT( nullptr != fm);
    BOOST_ASSERT( fm->active_fiber);
    BOOST_ASSERT( fm->active_fiber->is_running() );

    // set active-fiber to state_waiting
    fm->active_fiber->set_ready();
    // push active-fiber to fm->wqueue
    fm->wqueue.push( fm->active_fiber);
    // switch to another fiber
    fm_run();
    // fiber is resumed
}

void fm_join( detail::fiber_handle f) {
    fiber_manager * fm = detail::scheduler::instance();

    BOOST_ASSERT( nullptr != fm);
    BOOST_ASSERT( f);
    BOOST_ASSERT( f != fm->active_fiber);

    // set active-fiber to state_waiting
    fm->active_fiber->set_waiting();
    // push active-fiber to fm->wqueue
    fm->wqueue.push( fm->active_fiber);
    // add active-fiber to joinig-list of f
    if ( ! f->join( fm->active_fiber) ) {
        // f must be already terminated therefore we set
        // active-fiber to state_ready
        // FIXME: better state_running and no suspend
        fm->active_fiber->set_ready();
    }
    // switch to another fiber
    fm_run();
    // fiber is resumed

    BOOST_ASSERT( f->is_terminated() );
}

detail::fiber_handle fm_active() noexcept {
    fiber_manager * fm = detail::scheduler::instance();

    BOOST_ASSERT( nullptr != fm);

    return fm->active_fiber;
}

void fm_set_sched_algo( sched_algorithm * algo) {
    fiber_manager * fm = detail::scheduler::instance();

    BOOST_ASSERT( nullptr != fm);

    fm->sched_algo = algo;
}

void fm_wait_interval( std::chrono::high_resolution_clock::duration const& wait_interval) noexcept {
    fiber_manager * fm = detail::scheduler::instance();

    BOOST_ASSERT( nullptr != fm);

    fm->wait_interval = wait_interval;
}

std::chrono::high_resolution_clock::duration fm_wait_interval() noexcept {
    fiber_manager * fm = detail::scheduler::instance();

    BOOST_ASSERT( nullptr != fm);

    return fm->wait_interval;
}

bool fm_preserve_fpu() {
    fiber_manager * fm = detail::scheduler::instance();

    BOOST_ASSERT( nullptr != fm);

    return fm->preserve_fpu;
}

void fm_preserve_fpu( bool preserve) {
    fiber_manager * fm = detail::scheduler::instance();

    BOOST_ASSERT( nullptr != fm);

    fm->preserve_fpu == preserve;
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
