
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
	enter_mtx_( false),
    waiting_()
{}

void
manual_reset_event::wait()
{
	{
		mutex::scoped_lock lk( enter_mtx_);
        BOOST_ASSERT( lk);
		++waiters_;
	}

	while ( RESET == state_)
	{
	    if ( this_fiber::is_fiberized() )
        {
            waiting_.push_back(
                detail::scheduler::instance().active() );
            detail::scheduler::instance().active()->wait();
        }
        else
            detail::scheduler::instance().run();
	}

	if ( 0 == --waiters_)
		enter_mtx_.unlock();
}

bool
manual_reset_event::timed_wait( chrono::system_clock::time_point const& abs_time)
{
	{
		mutex::scoped_lock lk( enter_mtx_);
        BOOST_ASSERT( lk);
		++waiters_;
	}

	while ( RESET == state_)
	{
	    if ( this_fiber::is_fiberized() )
        {
            waiting_.push_back(
                detail::scheduler::instance().active() );
            detail::scheduler::instance().active()->sleep( abs_time);
        }
        else
            detail::scheduler::instance().run();
	}

	if ( 0 == --waiters_)
		enter_mtx_.unlock();
    return chrono::system_clock::now() <= abs_time;
}

bool
manual_reset_event::try_wait()
{
	{
		mutex::scoped_lock lk( enter_mtx_);
		BOOST_ASSERT( lk);
		++waiters_;
	}

	bool result = SET == state_;

	if ( 0 == --waiters_)
		enter_mtx_.unlock();

	return result;
}

void
manual_reset_event::set()
{
    mutex::scoped_lock lk( enter_mtx_);
    BOOST_ASSERT( lk);

    if ( RESET == state_)
    {
        state_ = SET;
        BOOST_FOREACH ( detail::fiber_base::ptr_t const& f, waiting_)
        { if ( ! f->is_complete() ) f->notify(); }
        waiting_.clear();
    }
}

void
manual_reset_event::reset()
{ state_ = RESET; }

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
