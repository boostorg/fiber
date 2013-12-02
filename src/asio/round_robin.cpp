
//   Copyright Christopher M. Kohlhoff, Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/fiber/asio/round_robin.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>

#include <boost/assert.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/scope_exit.hpp>
#include <boost/thread/locks.hpp>

#include "boost/fiber/detail/scheduler.hpp"
#include "boost/fiber/exceptions.hpp"
#include "boost/fiber/interruption.hpp"

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace asio {

round_robin::round_robin( boost::asio::io_service & svc) BOOST_NOEXCEPT :
    io_svc_( svc),
    active_fiber_(),
    wqueue_(),
    mn_()
{}

round_robin::~round_robin() BOOST_NOEXCEPT
{
    // fibers will be destroyed (stack-unwinding)
    // if last reference goes out-of-scope
    // therefore destructing wqueue_ && rqueue_
    // will destroy the fibers in this scheduler
    // if not referenced on other places
}

void
round_robin::spawn( detail::fiber_base::ptr_t const& f)
{
    BOOST_ASSERT( f);
    BOOST_ASSERT( f->is_ready() );

    // store active fiber in local var
    detail::fiber_base::ptr_t tmp = active_fiber_;
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

void
round_robin::evaluate_( detail::fiber_base::ptr_t const& f) {
    if ( f->is_waiting() )
        wqueue_.push_back(
            schedulable(
                f,
                boost::asio::io_service::work( io_svc_) ) );
    else if ( f->is_ready() ) spawn( f);
    else BOOST_ASSERT_MSG( false, "fiber with invalid state in ready-queue");
}

bool
round_robin::run()
{
    wqueue_t wqueue;
    // move all fibers witch are ready (state_ready)
    // from waiting-queue to the runnable-queue
    BOOST_FOREACH( schedulable const& s, wqueue_)
    {
        detail::fiber_base::ptr_t f( s.f);

        BOOST_ASSERT( ! f->is_running() );
        BOOST_ASSERT( ! f->is_terminated() );

        // set fiber to state_ready if dead-line was reached
        if ( s.tp <= clock_type::now() )
            f->set_ready();
        // set fiber to state_ready if interruption was requested
        if ( f->interruption_requested() )
            f->set_ready();
        if ( f->is_ready() )
        {
            io_svc_.post(
                boost::bind( & round_robin::evaluate_, this, s.f) );
        }
        else wqueue.push_back( s);
    }
    // exchange local with global waiting queue
    wqueue_.swap( wqueue);

    return 0 < io_svc_.poll_one();
}

void
round_robin::wait( unique_lock< detail::spinlock > & lk)
{ wait_until( clock_type::time_point( (clock_type::duration::max)() ), lk); }

bool
round_robin::wait_until( clock_type::time_point const& timeout_time,
                        unique_lock< detail::spinlock > & lk)
{
    clock_type::time_point start( clock_type::now() );

    BOOST_ASSERT( active_fiber_);
    BOOST_ASSERT( active_fiber_->is_running() );

    // set active_fiber to state_waiting
    active_fiber_->set_waiting();
    // reset the lock
    lk.unlock();
    // push active fiber to wqueue_
    wqueue_.push_back(
        schedulable(
            active_fiber_, timeout_time,
            boost::asio::io_service::work( io_svc_) ) );
    // store active fiber in local var
    detail::fiber_base::ptr_t tmp = active_fiber_;
    // suspend active fiber
    active_fiber_->suspend();
    // fiber is resumed

    BOOST_ASSERT( tmp == detail::scheduler::instance()->active() );
    BOOST_ASSERT( tmp->is_running() );

    return clock_type::now() < timeout_time;
}

void
round_robin::yield()
{
    BOOST_ASSERT( active_fiber_);
    BOOST_ASSERT( active_fiber_->is_running() );

    // set active fiber to state_waiting
    active_fiber_->set_ready();
    // push active fiber to wqueue_
    wqueue_.push_back(
        schedulable(
            active_fiber_,
            boost::asio::io_service::work( io_svc_) ) );
    // store active fiber in local var
    detail::fiber_base::ptr_t tmp = active_fiber_;
    // suspend acitive fiber
    active_fiber_->suspend();
    // fiber is resumed

    BOOST_ASSERT( tmp == detail::scheduler::instance()->active() );
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
        wqueue_.push_back(
            schedulable(
                active_fiber_,
                boost::asio::io_service::work( io_svc_) ) );
        // add active fiber to joinig-list of f
        if ( ! f->join( active_fiber_) )
            // f must be already terminated therefore we set
            // active fiber to state_ready
            // FIXME: better state_running and no suspend
            active_fiber_->set_ready();
        // store active fiber in local var
        detail::fiber_base::ptr_t tmp = active_fiber_;
        // suspend fiber until f terminates
        active_fiber_->suspend();
        // fiber is resumed by f

        BOOST_ASSERT( tmp == detail::scheduler::instance()->active() );
        BOOST_ASSERT( tmp->is_running() );
    }
    else
    {
        while ( ! f->is_terminated() )
            run();
    }

    BOOST_ASSERT( f->is_terminated() );
}

void
round_robin::priority( detail::fiber_base::ptr_t const& f, int prio)
{
    BOOST_ASSERT( f);

    // set only priority to fiber
    // round-robin does not respect priorities
    f->priority( prio);
}

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
