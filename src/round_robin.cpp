
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_wqueue_SOURCE

#include <boost/fiber/round_robin.hpp>

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>

#include <boost/assert.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/scope_exit.hpp>
#include <boost/thread/locks.hpp>

#include <boost/fiber/detail/scheduler.hpp>
#include <boost/fiber/exceptions.hpp>
#include <boost/fiber/interruption.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

round_robin::round_robin() :
    active_fiber_(),
    wqueue_(),
    rqueue_mtx_(),
    rqueue_()
{}

round_robin::~round_robin()
{
#if 0
    BOOST_ASSERT( ! active_fiber_);
    unique_lock< detail::spinlock > lk( rqueue_mtx_);
    BOOST_FOREACH( detail::fiber_base::ptr_t const& p, rqueue_)
    {
        p->release();
    }

    BOOST_FOREACH( detail::fiber_base::ptr_t const& p, wqueue_)
    {
        p->release();
    }
#endif
}

bool
round_robin::run()
{
    // loop over waiting queue
    wqueue_t wqueue;
    BOOST_FOREACH( detail::fiber_base::ptr_t const& f, wqueue_)
    {
        BOOST_ASSERT( ! f->is_running() );
        BOOST_ASSERT( ! f->is_terminated() );
        detail::state_t s = f->state();
        // set fiber to state_ready if interruption was requested
        // or the fiber was woken up
        if ( f->interruption_requested() || f->woken_up() ) f->set_ready();
        if ( f->is_ready() )
        {
            unique_lock< detail::spinlock > lk( rqueue_mtx_);
            rqueue_.push_back( f);
        }
        else wqueue.push_back( f);
    }
    // exchange local with global waiting queue
    wqueue_.swap( wqueue);

    // pop new fiber from ready-queue which is not complete
    // (example: fiber in ready-queue could be canceled by active-fiber)
    detail::fiber_base::ptr_t f;
    do
    {
        unique_lock< detail::spinlock > lk( rqueue_mtx_);
        if ( rqueue_.empty() ) return false;
        f.swap( rqueue_.front() );
        rqueue_.pop_front();
        lk.unlock();

        if ( f->is_ready() ) break;
        if ( f->is_waiting() ) wqueue_.push_back( f);
        else BOOST_ASSERT_MSG( false, "fiber with invalid state in ready-queue");
    }
    while ( true);

    detail::fiber_base::ptr_t tmp = active_fiber_;
    try
    {
        active_fiber_ = f;
        // resume new active fiber
        active_fiber_->set_running();
        active_fiber_->resume();
    }
    catch (...)
    {
        active_fiber_ = tmp;
        throw;
    }
    active_fiber_ = tmp;

    return true;
}

void
round_robin::wait( unique_lock< detail::spinlock > & lk)
{
    BOOST_ASSERT( active_fiber_);
    //FIXME: mabye other threads can change the state of active fiber?
    BOOST_ASSERT( active_fiber_->is_running() );

    // set active_fiber to state_waiting
    active_fiber_->set_waiting();
    // unlock assoc. sync. primitive
    lk.unlock();
    // push active fiber to wqueue_
    wqueue_.push_back( active_fiber_);
    // store active fiber in local var
    detail::fiber_base::ptr_t tmp = active_fiber_;
    // suspend fiber
    tmp->suspend();
    // fiber is resumed

    BOOST_ASSERT( tmp == active_fiber_);
    BOOST_ASSERT( tmp->is_running() );
}

void
round_robin::yield()
{
    BOOST_ASSERT( active_fiber_);
    //FIXME: mabye other threads can change the state of active fiber?
    BOOST_ASSERT( active_fiber_->is_running() );

    // set active fiber to state_waiting
    active_fiber_->set_waiting();
    // set active_fiber to state_ready
    active_fiber_->wake_up();
    // suspends active fiber and adds it to wqueue_
    // Note: adding to rqueue_ could result in a raise
    // between adding to rqueue_ and calling yield another
    // thread could steel fiber from rqueue_ and resume it
    // at the same time as yield is called
    wqueue_.push_back( active_fiber_);
    // store active fiber in local var
    detail::fiber_base::ptr_t tmp = active_fiber_;
    // suspend fiber
    tmp->suspend();
    // fiber is resumed

    BOOST_ASSERT( tmp == active_fiber_);
    BOOST_ASSERT( tmp->is_running() );
}

void
round_robin::join( detail::fiber_base::ptr_t const& f)
{
    BOOST_ASSERT( f);
    BOOST_ASSERT( f != active_fiber_);

    if ( active_fiber_)
    {
        // set active fiber to state_waiting
        active_fiber_->set_waiting();
        // push active fiber to wqueue_
        wqueue_.push_back( active_fiber_);
        // add active fiber to joinig-list of f
        if ( ! f->join( active_fiber_) )
            // f must be already terminated therefore we set
            // active fiber to state_ready
            // FIXME: better state_running and no suspend
            active_fiber_->wake_up();
        // store active fiber in local var
        detail::fiber_base::ptr_t tmp = active_fiber_;
        // suspend fiber until f terminates
        tmp->suspend();
        // fiber is resumed and by f
        BOOST_ASSERT( tmp->is_running() );

        // check if fiber was interrupted
        this_fiber::interruption_point();
    }
    else
    {
        while ( ! f->is_terminated() )
        {
            //FIXME: call this_thread::yield() before ?
            //FIXME: rethrow exception from f?
            run();
        }
    }

    // check if joined fiber has an exception
    // rethrow exception if YES
    if ( f->has_exception() ) f->rethrow();

    BOOST_ASSERT( f->is_terminated() );
}

void
round_robin::add( detail::fiber_base::ptr_t const& f)
{
    BOOST_ASSERT( f);
    BOOST_ASSERT( ! f->is_running() );
    BOOST_ASSERT( ! f->is_terminated() );

    wqueue_.push_back( f);
}

void
round_robin::priority( detail::fiber_base::ptr_t const& f, int prio)
{
    BOOST_ASSERT( f);

    f->priority( prio);
}

void
round_robin::migrate_to( fiber const& f)
{
    BOOST_ASSERT( f);

    add( detail::scheduler::extract( f) );
}

fiber
round_robin::steel_from()
{
    detail::fiber_base::ptr_t f;

    unique_lock< detail::spinlock > lk( rqueue_mtx_);
    if ( ! rqueue_.empty() )
    {
        f.swap( rqueue_.back() );
        rqueue_.pop_back();
    }
    lk.unlock();

    return fiber( f);
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
