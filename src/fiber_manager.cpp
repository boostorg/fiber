
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

bool fetch_ready( detail::worker_fiber * f)
{
    BOOST_ASSERT( ! f->is_running() );
    BOOST_ASSERT( ! f->is_terminated() );

    // set fiber to state_ready if dead-line was reached
    // set fiber to state_ready if interruption was requested
    if ( f->time_point() <= clock_type::now() || f->interruption_requested() )
        f->set_ready();
    return f->is_ready();
}

fiber_manager::fiber_manager() BOOST_NOEXCEPT :
    def_algo_( new round_robin() ),
    sched_algo_( def_algo_.get() ),
    wqueue_(),
    wait_interval_( chrono::milliseconds( 10) ),
    active_fiber_( 0)
{}

fiber_manager::~fiber_manager() BOOST_NOEXCEPT
{
    // fibers will be destroyed (stack-unwinding)
    // if last reference goes out-of-scope
    // therefore destructing fm->wqueue_ && rqueue_
    // will destroy the fibers in this scheduler
    // if not referenced on other places
    while ( ! wqueue_.empty() )
        fm_run();
}

void fm_resume_( detail::worker_fiber * f)
{
    volatile fiber_manager * fm = detail::scheduler::instance();

    BOOST_ASSERT( 0 != fm);
    BOOST_ASSERT( 0 != f);
    BOOST_ASSERT( f->is_ready() );

    // fiber to state_running
    f->set_running();
    // check if active-fiber calls itself
    // this might happend if fiber calls yield() and no
    // other fiber is in the ready-queue
    if ( f != fm->active_fiber_)
    {
        // store active-fiber in local var
        detail::worker_fiber * tmp = fm->active_fiber_;
        // assign new fiber to active-fiber
        fm->active_fiber_ = f;
        // resume active-fiber == start or yield to
        fm->active_fiber_->resume( tmp);
        // if fiber was migrated to another thread
        // the fiber-manger pointer, allocated on the stack,
        // is invalid
        fm = detail::scheduler::instance();
        // if active-fiber is detached and has terminated
        // the fiber has to be destructed/deallocated
        if ( 0 != fm->active_fiber_ &&
             fm->active_fiber_->detached() &&
             fm->active_fiber_->is_terminated() )
            fm->active_fiber_->deallocate();
        fm->active_fiber_ = tmp;
    }
}

void fm_set_sched_algo( sched_algorithm * algo)
{
    fiber_manager * fm = detail::scheduler::instance();

    BOOST_ASSERT( 0 != fm);
    
    fm->sched_algo_ = algo;
    fm->def_algo_.reset();
}

clock_type::time_point fm_next_wakeup()
{
    fiber_manager * fm = detail::scheduler::instance();

    BOOST_ASSERT( 0 != fm);

    if ( fm->wqueue_.empty() )
        return clock_type::now() + fm->wait_interval_;
    else
    {
        clock_type::time_point wakeup( fm->wqueue_.top()->time_point() );
        if ( (clock_type::time_point::max)() == wakeup)
            return clock_type::now() + fm->wait_interval_;
        return wakeup;
    }
}

void fm_spawn( detail::worker_fiber * f)
{
    fiber_manager * fm = detail::scheduler::instance();

    BOOST_ASSERT( 0 != fm);
    BOOST_ASSERT( 0 != f);
    BOOST_ASSERT( f->is_ready() );

    fm->sched_algo_->awakened( f);
}

void fm_priority( detail::worker_fiber * f,
                  int prio) BOOST_NOEXCEPT
{
    fiber_manager * fm = detail::scheduler::instance();

    BOOST_ASSERT( 0 != fm);

    fm->sched_algo_->priority( f, prio);
}

void fm_wait_interval( clock_type::duration const& wait_interval) BOOST_NOEXCEPT
{
    fiber_manager * fm = detail::scheduler::instance();

    BOOST_ASSERT( 0 != fm);

    fm->wait_interval_ = wait_interval;
}

clock_type::duration fm_wait_interval() BOOST_NOEXCEPT
{
    fiber_manager * fm = detail::scheduler::instance();

    BOOST_ASSERT( 0 != fm);

    return fm->wait_interval_;
}

void fm_run()
{
    fiber_manager * fm = detail::scheduler::instance();

    BOOST_ASSERT( 0 != fm);

    // move all fibers witch are ready (state_ready)
    // from waiting-queue to the runnable-queue
    fm->wqueue_.move_to( fm->sched_algo_, fetch_ready);

    // pop new fiber from ready-queue which is not complete
    // (example: fiber in ready-queue could be canceled by active-fiber)
    detail::worker_fiber * f( fm->sched_algo_->pick_next() );
    if ( f)
    {
        BOOST_ASSERT_MSG( f->is_ready(), "fiber with invalid state in ready-queue");
        fm_resume_( f);
    }
    else
    {
        if ( fm->active_fiber_)
            fm->active_fiber_->suspend();
        else
        {
            // no fibers ready to run; the thread should sleep
            // until earliest fiber is scheduled to run
//           clock_type::time_point wakeup( fm_next_wakeup() );
//           this_thread::sleep_until( wakeup);
        }
    }
}

void fm_wait( unique_lock< detail::spinlock > & lk)
{
    fm_wait_until( clock_type::time_point( (clock_type::duration::max)() ), lk);
}

bool fm_wait_until( clock_type::time_point const& timeout_time,
                    unique_lock< detail::spinlock > & lk)
{
    fiber_manager * fm = detail::scheduler::instance();

    BOOST_ASSERT( 0 != fm);

    clock_type::time_point start( clock_type::now() );

    BOOST_ASSERT( fm->active_fiber_);
    BOOST_ASSERT( fm->active_fiber_->is_running() );

    // set active-fiber to state_waiting
    fm->active_fiber_->set_waiting();
    // release lock
    lk.unlock();
    // push active-fiber to fm->wqueue_
    fm->active_fiber_->time_point( timeout_time);
    fm->wqueue_.push( fm->active_fiber_);
    // run next fiber
    fm_run();

    return clock_type::now() < timeout_time;
}

void fm_yield()
{
    fiber_manager * fm = detail::scheduler::instance();

    BOOST_ASSERT( 0 != fm);
    BOOST_ASSERT( 0 != fm->active_fiber_);
    BOOST_ASSERT( fm->active_fiber_->is_running() );

    // set active-fiber to state_waiting
    fm->active_fiber_->set_ready();
    // push active-fiber to scheduler-algo
    fm->sched_algo_->awakened( fm->active_fiber_);
    // run next fiber
    fm_run();
}

void fm_join( detail::worker_fiber * f)
{
    fiber_manager * fm = detail::scheduler::instance();

    BOOST_ASSERT( 0 != fm);
    BOOST_ASSERT( 0 != f);
    BOOST_ASSERT( f != fm->active_fiber_);

    if ( fm->active_fiber_)
    {
        // set active-fiber to state_waiting
        fm->active_fiber_->set_waiting();
        // push active-fiber to fm->wqueue_
        fm->wqueue_.push( fm->active_fiber_);
        // add active-fiber to joinig-list of f
        if ( ! f->join( fm->active_fiber_) )
            // f must be already terminated therefore we set
            // active-fiber to state_ready
            // FIXME: better state_running and no suspend
            fm->active_fiber_->set_ready();
        // run next fiber
        fm_run();
    }
    else
    {
        while ( ! f->is_terminated() )
            // yield this thread if scheduler did not 
            // resumed some fibers in the previous round
            fm_run();
    }

    BOOST_ASSERT( f->is_terminated() );
}

detail::worker_fiber * fm_active() BOOST_NOEXCEPT
{
    fiber_manager * fm = detail::scheduler::instance();

    BOOST_ASSERT( 0 != fm);

    return fm->active_fiber_;
}

void fm_migrate( detail::worker_fiber * f)
{
    BOOST_ASSERT( 0 != f);
    BOOST_ASSERT( f->is_ready() );

    fm_spawn( f);
    fm_run();
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
