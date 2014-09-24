
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/fiber/fiber_manager.hpp>

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>

#include <boost/assert.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/scope_exit.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/thread.hpp>

#include <boost/fiber/detail/scheduler.hpp>
#include <boost/fiber/exceptions.hpp>

#include <boost/fiber/round_robin.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

bool fetch_ready( detail::fiber_base * f, chrono::high_resolution_clock::time_point const& now)
{
    BOOST_ASSERT( ! f->is_running() );
    BOOST_ASSERT( ! f->is_terminated() );

    // set fiber to state_ready if dead-line was reached
    // set fiber to state_ready if interruption was requested
    if ( f->time_point() <= now || f->interruption_requested() )
        f->set_ready();
    return f->is_ready();
}

fiber_manager::fiber_manager() BOOST_NOEXCEPT :
    def_algo( new round_robin() ),
    sched_algo( def_algo.get() ),
    wqueue(),
    preserve_fpu( false),
    main_fiber_(),
    wait_interval( chrono::milliseconds( 10) ),
    active_fiber( & main_fiber_)
{}

fiber_manager::~fiber_manager() BOOST_NOEXCEPT
{
    // fibers will be destroyed (stack-unwinding)
    // if last reference goes out-of-scope
    // therefore destructing fm->wqueue && rqueue_
    // will destroy the fibers in this scheduler
    // if not referenced on other places
    while ( ! wqueue.empty() )
        fm_run();
}

void fm_resume_( detail::fiber_base * f)
{
    fiber_manager * fm = detail::scheduler::instance();

    BOOST_ASSERT( 0 != fm);
    BOOST_ASSERT( 0 != f);
    BOOST_ASSERT( f->is_ready() );

    // fiber to state_running
    f->set_running();

    if ( f == fm->active_fiber) return;

    // store active-fiber in local var
    detail::fiber_base * tmp = fm->active_fiber;
    // assign new fiber to active-fiber
    fm->active_fiber = f;
    // if active-fiber is detached and has terminated
    // the fiber has to be destructed/deallocated
    BOOST_ASSERT( 0 != tmp);
    if ( tmp->is_terminated() )
        intrusive_ptr_release( tmp);
    // resume active-fiber == start or yield to
    fm->active_fiber->resume( tmp, fm->preserve_fpu);
}

void fm_set_sched_algo( sched_algorithm * algo)
{
    fiber_manager * fm = detail::scheduler::instance();

    BOOST_ASSERT( 0 != fm);

    fm->sched_algo = algo;
    fm->def_algo.reset();
}

chrono::high_resolution_clock::time_point fm_next_wakeup()
{
    fiber_manager * fm = detail::scheduler::instance();

    BOOST_ASSERT( 0 != fm);

    if ( fm->wqueue.empty() )
        return chrono::high_resolution_clock::now() + fm->wait_interval;
    else
    {
        //FIXME: search for the closest time_point to now() in waiting-queue
        chrono::high_resolution_clock::time_point wakeup( fm->wqueue.top()->time_point() );
        if ( (chrono::high_resolution_clock::time_point::max)() == wakeup)
            return chrono::high_resolution_clock::now() + fm->wait_interval;
        return wakeup;
    }
}

void fm_spawn( detail::fiber_base * f)
{
    fiber_manager * fm = detail::scheduler::instance();

    BOOST_ASSERT( 0 != fm);
    BOOST_ASSERT( 0 != f);
    BOOST_ASSERT( f->is_ready() );
    BOOST_ASSERT( f != fm->active_fiber);

    fm->sched_algo->awakened( f);
}

void fm_priority( detail::fiber_base * f,
                  int prio) BOOST_NOEXCEPT
{
    fiber_manager * fm = detail::scheduler::instance();

    BOOST_ASSERT( 0 != fm);

    fm->sched_algo->priority( f, prio);
}

void fm_wait_interval( chrono::high_resolution_clock::duration const& wait_interval) BOOST_NOEXCEPT
{
    fiber_manager * fm = detail::scheduler::instance();

    BOOST_ASSERT( 0 != fm);

    fm->wait_interval = wait_interval;
}

chrono::high_resolution_clock::duration fm_wait_interval() BOOST_NOEXCEPT
{
    fiber_manager * fm = detail::scheduler::instance();

    BOOST_ASSERT( 0 != fm);

    return fm->wait_interval;
}

void fm_run()
{
    fiber_manager * fm = detail::scheduler::instance();

    BOOST_ASSERT( 0 != fm);

    for (;;)
    {
        // move all fibers witch are ready (state_ready)
        // from waiting-queue to the runnable-queue
        fm->wqueue.move_to( fm->sched_algo, fetch_ready);

        // pop new fiber from ready-queue
        detail::fiber_base * f( fm->sched_algo->pick_next() );
        if ( f)
        {
            BOOST_ASSERT_MSG( f->is_ready(), "fiber with invalid state in ready-queue");
            fm_resume_( f);
            return;
        }
        else
        {
            // no fibers ready to run; the thread should sleep
            // until earliest fiber is scheduled to run
            chrono::high_resolution_clock::time_point wakeup( fm_next_wakeup() );
            this_thread::sleep_until( wakeup);
        }
    }
}

void fm_wait( unique_lock< detail::spinlock > & lk)
{
    fm_wait_until( chrono::high_resolution_clock::time_point( (chrono::high_resolution_clock::duration::max)() ), lk);
}

bool fm_wait_until( chrono::high_resolution_clock::time_point const& timeout_time,
                    unique_lock< detail::spinlock > & lk)
{
    fiber_manager * fm = detail::scheduler::instance();

    BOOST_ASSERT( 0 != fm);
    BOOST_ASSERT( 0 != fm->active_fiber);
    BOOST_ASSERT( fm->active_fiber->is_running() );

    chrono::high_resolution_clock::time_point start( chrono::high_resolution_clock::now() );

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

    return chrono::high_resolution_clock::now() < timeout_time;
}

void fm_yield()
{
    fiber_manager * fm = detail::scheduler::instance();

    BOOST_ASSERT( 0 != fm);
    BOOST_ASSERT( 0 != fm->active_fiber);
    BOOST_ASSERT( fm->active_fiber->is_running() );

    // set active-fiber to state_waiting
    fm->active_fiber->set_ready();
    // push active-fiber to fm->wqueue
    fm->wqueue.push( fm->active_fiber);
    // switch to another fiber
    fm_run();
    // fiber is resumed
}

void fm_join( detail::fiber_base * f)
{
    fiber_manager * fm = detail::scheduler::instance();

    BOOST_ASSERT( 0 != fm);
    BOOST_ASSERT( 0 != f);
    BOOST_ASSERT( f != fm->active_fiber);

    // set active-fiber to state_waiting
    fm->active_fiber->set_waiting();
    // push active-fiber to fm->wqueue
    fm->wqueue.push( fm->active_fiber);
    // add active-fiber to joinig-list of f
    if ( ! f->join( fm->active_fiber) )
        // f must be already terminated therefore we set
        // active-fiber to state_ready
        // FIXME: better state_running and no suspend
        fm->active_fiber->set_ready();
    // switch to another fiber
    fm_run();
    // fiber is resumed

    BOOST_ASSERT( f->is_terminated() );
}

detail::fiber_base * fm_active() BOOST_NOEXCEPT
{
    fiber_manager * fm = detail::scheduler::instance();

    BOOST_ASSERT( 0 != fm);

    return fm->active_fiber;
}

void fm_migrate( detail::fiber_base * f)
{
    BOOST_ASSERT( 0 != f);
    BOOST_ASSERT( f->is_ready() );

    fm_spawn( f);
    fm_run();
}

bool fm_preserve_fpu_()
{
    fiber_manager * fm = detail::scheduler::instance();

    BOOST_ASSERT( 0 != fm);

    return fm->preserve_fpu;
}

void fm_preserve_fpu_( bool preserve_fpu_)
{
    fiber_manager * fm = detail::scheduler::instance();

    BOOST_ASSERT( 0 != fm);

    fm->preserve_fpu == preserve_fpu_;
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
