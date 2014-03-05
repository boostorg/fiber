
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "workstealing_round_robin.hpp"

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

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

workstealing_round_robin::workstealing_round_robin() BOOST_NOEXCEPT :
    active_fiber_(),
    wqueue_(),
    rqueue_(),
    mn_()
{}

workstealing_round_robin::~workstealing_round_robin() BOOST_NOEXCEPT
{
    // fibers will be destroyed (stack-unwinding)
    // if last reference goes out-of-scope
    // therefore destructing wqueue_ && rqueue_
    // will destroy the fibers in this scheduler
    // if not referenced on other places
    while ( ! wqueue_.empty() && ! rqueue_.empty() )
        run();
}

void
workstealing_round_robin::spawn( boost::fibers::detail::worker_fiber::ptr_t const& f)
{
    BOOST_ASSERT( f);
    BOOST_ASSERT( f->is_ready() );

    // store active fiber in local var
    boost::fibers::detail::worker_fiber::ptr_t tmp = active_fiber_;
    // assign new fiber to active fiber
    active_fiber_ = f;
    // set active fiber to state_running
    active_fiber_->set_running();
    // resume active fiber
    active_fiber_->resume();
    // fiber is resumed

    BOOST_ASSERT( f == active_fiber_);
    // reset active fiber to previous
    active_fiber_ = tmp;
}

bool
workstealing_round_robin::run()
{
    wqueue_t wqueue;
    // move all fibers witch are ready (state_ready)
    // from waiting-queue to the runnable-queue
    BOOST_FOREACH( schedulable const& s, wqueue_)
    {
        boost::fibers::detail::worker_fiber::ptr_t f( s.f);

        BOOST_ASSERT( ! f->is_running() );
        BOOST_ASSERT( ! f->is_terminated() );

        // set fiber to state_ready if dead-line was reached
        if ( s.tp <= boost::fibers::clock_type::now() )
            f->set_ready();
        // set fiber to state_ready if interruption was requested
        if ( f->interruption_requested() )
            f->set_ready();
        if ( f->is_ready() )
            rqueue_.push( f);
        else wqueue.push_back( s);
    }
    // exchange local with global waiting queue
    wqueue_.swap( wqueue);

    // pop new fiber from ready-queue which is not complete
    // (example: fiber in ready-queue could be canceled by active-fiber)
    boost::fibers::detail::worker_fiber::ptr_t f;
    do
    {
        if ( ! rqueue_.try_pop( f) )
        {
            boost::this_thread::yield();
            return false;
        }
        if ( f->is_ready() ) break;
        else BOOST_ASSERT_MSG( false, "fiber with invalid state in ready-queue");
    }
    while ( true);

    // resume fiber
    spawn( f);

    return true;
}

void
workstealing_round_robin::wait( boost::unique_lock< boost::fibers::detail::spinlock > & lk)
{ wait_until( boost::fibers::clock_type::time_point( (boost::fibers::clock_type::duration::max)() ), lk); }

bool
workstealing_round_robin::wait_until( boost::fibers::clock_type::time_point const& timeout_time,
                                      boost::unique_lock< boost::fibers::detail::spinlock > & lk)
{
    boost::fibers::clock_type::time_point start( boost::fibers::clock_type::now() );

    BOOST_ASSERT( active_fiber_);
    BOOST_ASSERT( active_fiber_->is_running() );

    // set active fiber to state_waiting
    active_fiber_->set_waiting();
    // release lock
    lk.unlock();
    // push active fiber to wqueue_
    wqueue_.push_back( schedulable( active_fiber_, timeout_time) );
    // store active fiber in local var
    boost::fibers::detail::worker_fiber::ptr_t tmp = active_fiber_;
    // suspend active fiber
    active_fiber_->suspend();
    // fiber is resumed

    BOOST_ASSERT( tmp == boost::fibers::detail::scheduler::instance()->active() );
    BOOST_ASSERT( tmp->is_running() );

    return boost::fibers::clock_type::now() < timeout_time;
}

void
workstealing_round_robin::yield()
{
    BOOST_ASSERT( active_fiber_);
    BOOST_ASSERT( active_fiber_->is_running() );

    // set active fiber to state_waiting
    active_fiber_->set_ready();
    // push active fiber to wqueue_
    wqueue_.push_back( schedulable( active_fiber_) );
    // store active fiber in local var
    boost::fibers::detail::worker_fiber::ptr_t tmp = active_fiber_;
    // suspend acitive fiber
    active_fiber_->suspend();
    // fiber is resumed

    BOOST_ASSERT( tmp == boost::fibers::detail::scheduler::instance()->active() );
    BOOST_ASSERT( tmp->is_running() );
}

void
workstealing_round_robin::join( boost::fibers::detail::worker_fiber::ptr_t const& f)
{
    BOOST_ASSERT( f);
    BOOST_ASSERT( f != active_fiber_);

    if ( active_fiber_)
    {
        // set active fiber to state_waiting
        active_fiber_->set_waiting();
        // push active fiber to wqueue_
        wqueue_.push_back( schedulable( active_fiber_) );
        // add active fiber to joinig-list of f
        if ( ! f->join( active_fiber_) )
            // f must be already terminated therefore we set
            // active fiber to state_ready
            // FIXME: better state_running and no suspend
            active_fiber_->set_ready();
        // store active fiber in local var
        boost::fibers::detail::worker_fiber::ptr_t tmp = active_fiber_;
        // suspend fiber until f terminates
        active_fiber_->suspend();
        // fiber is resumed by f

        BOOST_ASSERT( tmp == boost::fibers::detail::scheduler::instance()->active() );
        BOOST_ASSERT( tmp->is_running() );
    }
    else
    {
        while ( ! f->is_terminated() )
        {
            // yield this thread if scheduler did not 
            // resumed some fibers in the previous round
            if ( ! run() ) boost::this_thread::yield();
        }
    }

    BOOST_ASSERT( f->is_terminated() );
}

void
workstealing_round_robin::priority( boost::fibers::detail::worker_fiber::ptr_t const& f, int prio) BOOST_NOEXCEPT
{
    BOOST_ASSERT( f);

    // set only priority to fiber
    // round-robin does not respect priorities
    f->priority( prio);
}

boost::fibers::fiber
workstealing_round_robin::steal_from()
{
    boost::fibers::detail::worker_fiber::ptr_t f;
    if ( rqueue_.try_pop( f) ) return boost::fibers::fiber( f);
    return boost::fibers::fiber();
}

void
workstealing_round_robin::migrate_to( boost::fibers::fiber const& f)
{
    BOOST_ASSERT( f);

    spawn( boost::fibers::detail::scheduler::extract( f) );
}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
