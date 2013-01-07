
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_FIBERS_SOURCE

#include <boost/fiber/mutex.hpp>

#include <boost/assert.hpp>

#include <boost/fiber/detail/scheduler.hpp>
#include <boost/fiber/operations.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

mutex::mutex() :
	state_( UNLOCKED),
    mtx_(),
    waiting_()
{}

void
mutex::lock()
{
    BOOST_ASSERT( this_fiber::is_fiberized() );

    while ( LOCKED == state_.exchange( LOCKED, memory_order_acquire) )
    {
        detail::spin_mutex::scoped_lock lk( mtx_);
        waiting_.push_back(
                detail::scheduler::instance().active() );
        detail::scheduler::instance().wait( lk);
    }
}

bool
mutex::try_lock()
{
    BOOST_ASSERT( this_fiber::is_fiberized() );

    return UNLOCKED == state_.exchange( LOCKED, memory_order_acquire);
}

void
mutex::unlock()
{
    BOOST_ASSERT( this_fiber::is_fiberized() );

	state_ = UNLOCKED;

    detail::spin_mutex::scoped_lock lk( mtx_);
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
