
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_FIBERS_SOURCE

#include "boost/fiber/manual_reset_event.hpp"

#include <boost/assert.hpp>
#include <boost/foreach.hpp>

#include <boost/fiber/detail/scheduler.hpp>
#include <boost/fiber/operations.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

manual_reset_event::manual_reset_event( bool isset) :
    state_( isset ? SET : RESET),
    waiters_( 0),
    enter_mtx_(),
    waiting_mtx_(),
    waiting_()
{}

void
manual_reset_event::wait()
{
    BOOST_ASSERT( this_fiber::is_fiberized() );

    {
        mutex::scoped_lock lk( enter_mtx_);
        BOOST_ASSERT( lk);
        ++waiters_;
    }

    while ( RESET == state_.exchange( RESET, memory_order_acquire) )
    {
        detail::spin_mutex::scoped_lock lk( waiting_mtx_);
        waiting_.push_back(
                detail::scheduler::instance().active() );
        detail::scheduler::instance().wait( lk);
    }

    if ( 0 == --waiters_)
        enter_mtx_.unlock();
}

bool
manual_reset_event::timed_wait( chrono::system_clock::time_point const& abs_time)
{
    BOOST_ASSERT( this_fiber::is_fiberized() );

    if ( chrono::system_clock::now() >= abs_time) return false;

    {
        mutex::scoped_lock lk( enter_mtx_);
        BOOST_ASSERT( lk);
        ++waiters_;
    }

    while ( RESET == state_.exchange( RESET, memory_order_acquire) )
    {
        detail::spin_mutex::scoped_lock lk( waiting_mtx_);
        waiting_.push_back(
                detail::scheduler::instance().active() );
        detail::scheduler::instance().wait( lk);

        if ( chrono::system_clock::now() >= abs_time) return false;
    }

    if ( 0 == --waiters_)
        enter_mtx_.unlock();

    return true;
}

bool
manual_reset_event::try_wait()
{
    BOOST_ASSERT( this_fiber::is_fiberized() );

    {
        mutex::scoped_lock lk( enter_mtx_);
        BOOST_ASSERT( lk);
        ++waiters_;
    }

    bool result = SET == state_.exchange( RESET, memory_order_acquire);

    if ( 0 == --waiters_)
        enter_mtx_.unlock();

    return result;
}

void
manual_reset_event::set()
{
    BOOST_ASSERT( this_fiber::is_fiberized() );

    mutex::scoped_lock lk( enter_mtx_);
    BOOST_ASSERT( lk);

    if ( RESET == state_.exchange( SET, memory_order_acquire) )
    {
        detail::spin_mutex::scoped_lock lk( waiting_mtx_);
        BOOST_FOREACH ( detail::fiber_base::ptr_t const& f, waiting_)
        { f->set_ready(); }
        waiting_.clear();
    }
}

void
manual_reset_event::reset()
{
    BOOST_ASSERT( this_fiber::is_fiberized() );

    state_ = RESET;
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
