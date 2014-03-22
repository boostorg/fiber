
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

void
fiber_manager::resume_( detail::worker_fiber * f)
{
    BOOST_ASSERT( f);
    BOOST_ASSERT( f->is_ready() );

    // store active fiber in local var
    detail::worker_fiber * tmp( active_fiber_);
    // assign new fiber to active fiber
    active_fiber_ = f;
    // set active fiber to state_running
    active_fiber_->set_running();
    // check if active-fiber calls itself
    // this might happend if fiber calls yield() and no
    // other fiber is in the ready-queue
    if ( tmp != active_fiber_)
    {
        // resume active-fiber == start or yield to
        active_fiber_->resume( tmp);
        if ( active_fiber_->detached() && active_fiber_->is_terminated() )
            active_fiber_->deallocate();
        // reset active fiber to previous
        active_fiber_ = tmp;
    }
}

clock_type::time_point
fiber_manager::next_wakeup_()
{
    if ( wqueue_.empty() )
        return clock_type::now() + wait_interval_;
    else
    {
        clock_type::time_point wakeup( wqueue_.top()->time_point() );
        if ( (clock_type::time_point::max)() == wakeup)
            return clock_type::now() + wait_interval_;
        return wakeup;
    }
}

fiber_manager::fiber_manager() BOOST_NOEXCEPT :
    def_algo_( new round_robin() ),
    sched_algo_( def_algo_.get() ),
    wait_interval_( chrono::milliseconds( 10) ),
    active_fiber_( 0),
    wqueue_()
{}

fiber_manager::~fiber_manager() BOOST_NOEXCEPT
{
    // fibers will be destroyed (stack-unwinding)
    // if last reference goes out-of-scope
    // therefore destructing wqueue_ && rqueue_
    // will destroy the fibers in this scheduler
    // if not referenced on other places
    
    //active_fiber_->set_terminated();

    while ( ! wqueue_.empty() )
        run();
}

void
fiber_manager::spawn( detail::worker_fiber * f)
{
    BOOST_ASSERT( f);
    BOOST_ASSERT( f->is_ready() );

    sched_algo_->awakened( f);
}

void
fiber_manager::run()
{
    for (;;)
    {
        // move all fibers witch are ready (state_ready)
        // from waiting-queue to the runnable-queue
        wqueue_.move_to( sched_algo_, fetch_ready);

        // pop new fiber from ready-queue which is not complete
        // (example: fiber in ready-queue could be canceled by active-fiber)
        detail::worker_fiber * f( sched_algo_->pick_next() );
        if ( f)
        {
            BOOST_ASSERT_MSG( f->is_ready(), "fiber with invalid state in ready-queue");
            resume_( f);
            return;
        }
        else
        {
#if 0
            if ( active_fiber_)
                active_fiber_->suspend();
            else
                this_thread::yield();
            return;
#endif
            if ( active_fiber_)
                active_fiber_->suspend();
            else
            {
                // no fibers ready to run; the thread should sleep
                // until earliest fiber is scheduled to run
                clock_type::time_point wakeup( next_wakeup_() );
                this_thread::sleep_until( wakeup);
            }
            return;
        }
    }
}

void
fiber_manager::wait( unique_lock< detail::spinlock > & lk)
{ wait_until( clock_type::time_point( (clock_type::duration::max)() ), lk); }

bool
fiber_manager::wait_until( clock_type::time_point const& timeout_time,
                           unique_lock< detail::spinlock > & lk)
{
    clock_type::time_point start( clock_type::now() );

    BOOST_ASSERT( active_fiber_);
    BOOST_ASSERT( active_fiber_->is_running() );

    // set active fiber to state_waiting
    active_fiber_->set_waiting();
    // release lock
    lk.unlock();
    // push active fiber to wqueue_
    active_fiber_->time_point( timeout_time);
    wqueue_.push( active_fiber_);
    // run next fiber
    run();

    return clock_type::now() < timeout_time;
}

void
fiber_manager::yield()
{
    BOOST_ASSERT( active_fiber_);
    BOOST_ASSERT( active_fiber_->is_running() );

    // set active fiber to state_waiting
    active_fiber_->set_ready();
    // push active fiber to wqueue_
    wqueue_.push( active_fiber_);
    // run next fiber
    run();
}

void
fiber_manager::join( detail::worker_fiber * f)
{
    BOOST_ASSERT( f);
    BOOST_ASSERT( f != active_fiber_);

    if ( active_fiber_)
    {
        // set active fiber to state_waiting
        active_fiber_->set_waiting();
        // push active fiber to wqueue_
        wqueue_.push( active_fiber_);
        // add active fiber to joinig-list of f
        if ( ! f->join( active_fiber_) )
            // f must be already terminated therefore we set
            // active fiber to state_ready
            // FIXME: better state_running and no suspend
            active_fiber_->set_ready();
        // run next fiber
        run();
    }
    else
    {
        while ( ! f->is_terminated() )
            // yield this thread if scheduler did not 
            // resumed some fibers in the previous round
            run();
    }

    BOOST_ASSERT( f->is_terminated() );
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
