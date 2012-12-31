
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

mutex::mutex( bool checked) :
	state_( UNLOCKED),
    owner_(),
    waiting_(),
    checked_( checked)
{}

void
mutex::lock()
{
    while ( LOCKED == state_.exchange( LOCKED, memory_order_acquire) )
    {
        if ( this_fiber::is_fiberized() )
        {
            waiting_.push_back(
                    detail::scheduler::instance().active() );
            detail::scheduler::instance().wait();
        }
        else
            detail::scheduler::instance().run();
    }
    if ( this_fiber::is_fiberized() )
        owner_ = detail::scheduler::instance().active()->get_id();
    else
        owner_ = detail::fiber_base::id();
}

bool
mutex::try_lock()
{
    if ( LOCKED == state_.exchange( LOCKED, memory_order_acquire) ) return false;
    if ( this_fiber::is_fiberized() )
        owner_ = detail::scheduler::instance().active()->get_id();
    else
        owner_ = detail::fiber_base::id();
    return true;
}

void
mutex::unlock()
{
    if ( checked_)
    {
        if ( this_fiber::is_fiberized() )
        {
            if ( detail::scheduler::instance().active()->get_id() != owner_)
                std::abort();
        }
        else if ( detail::fiber_base::id() != owner_)
                std::abort();
    }

    owner_ = detail::fiber_base::id();
	state_.store( UNLOCKED);

	if ( ! waiting_.empty() )
    {
        detail::fiber_base::ptr_t f;
        do
        {
            f.swap( waiting_.front() );
            waiting_.pop_front();
        } while ( f->is_terminated() );
        if ( f)
            detail::scheduler::instance().notify( f);
    }
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
