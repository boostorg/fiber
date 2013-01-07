
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_FIBERS_SOURCE

#include "boost/fiber/auto_reset_event.hpp"

#include <boost/assert.hpp>

#include <boost/fiber/detail/fiber_base.hpp>
#include <boost/fiber/detail/scheduler.hpp>
#include <boost/fiber/operations.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

auto_reset_event::auto_reset_event( bool isset) :
    state_( isset ? SET : RESET),
    waiting_mtx_(),
    waiting_()
{}

void
auto_reset_event::wait()
{
    BOOST_ASSERT( this_fiber::is_fiberized() );

    while ( RESET == state_.exchange( RESET, memory_order_acquire) )
    {
        detail::spin_mutex::scoped_lock lk( waiting_mtx_);
        waiting_.push_back(
            detail::scheduler::instance().active() );
        detail::scheduler::instance().wait( lk);
    }
}

bool
auto_reset_event::timed_wait( chrono::system_clock::time_point const& abs_time)
{
    BOOST_ASSERT_MSG( false, "not implemented");

    BOOST_ASSERT( this_fiber::is_fiberized() );

    if ( chrono::system_clock::now() >= abs_time) return false;

    while ( RESET == state_.exchange( RESET, memory_order_acquire) )
    {
        detail::spin_mutex::scoped_lock lk( waiting_mtx_);
        waiting_.push_back(
            detail::scheduler::instance().active() );
        detail::scheduler::instance().wait( lk);

        if ( chrono::system_clock::now() >= abs_time) return false;
    }

    return true;
}

bool
auto_reset_event::try_wait()
{
    BOOST_ASSERT( this_fiber::is_fiberized() );

    return SET == state_.exchange( RESET, memory_order_acquire);
}

void
auto_reset_event::set()
{
    BOOST_ASSERT( this_fiber::is_fiberized() );

    state_ = SET;

    detail::spin_mutex::scoped_lock lk( waiting_mtx_);
	if ( ! waiting_.empty() )
    {
        detail::fiber_base::ptr_t f;
        f.swap( waiting_.front() );
        waiting_.pop_front();
        f->set_ready();
    }
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
