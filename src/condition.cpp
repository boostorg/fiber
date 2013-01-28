
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_FIBERS_SOURCE

#include "boost/fiber/condition.hpp"

#include <boost/foreach.hpp>

#include <boost/fiber/detail/scheduler.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

condition::condition() :
	cmd_( SLEEPING),
	waiters_( 0),
	enter_mtx_(),
	check_mtx_(),
    waiting_mtx_(),
    waiting_()
{}

condition::~condition()
{ BOOST_ASSERT( 0 == waiters_); }

void
condition::notify_one()
{
	enter_mtx_.lock();

	if ( 0 == waiters_)
	{
		enter_mtx_.unlock();
		return;
	}

    command expected = SLEEPING;
    while ( ! cmd_.compare_exchange_strong( expected, NOTIFY_ONE) )
    {
        if ( this_fiber::is_fiberized() ) this_fiber::yield();
        expected = SLEEPING;
    }

    unique_lock< detail::spinlock > lk( waiting_mtx_);
	if ( ! waiting_.empty() )
    {
        waiting_.front()->wake_up();
        waiting_.pop_front();
    }
}

void
condition::notify_all()
{
    //This mutex guarantees that no other thread can enter to the
    //do_timed_wait method logic, so that thread count will be
    //constant until the function writes a NOTIFY_ALL command.
    //It also guarantees that no other notification can be signaled
    //on this spin_condition before this one ends
	enter_mtx_.lock();

    //Return if there are no waiters
	if ( 0 == waiters_)
	{
		enter_mtx_.unlock();
		return;
	}

    //Notify that all threads should execute wait logic
    command expected = SLEEPING;
    while ( SLEEPING != cmd_.compare_exchange_strong( expected, NOTIFY_ALL) )
    {
        if ( this_fiber::is_fiberized() ) this_fiber::yield();
        expected = SLEEPING;
    }

    unique_lock< detail::spinlock > lk( waiting_mtx_);
	BOOST_FOREACH( detail::fiber_base::ptr_t const& f, waiting_)
    { f->wake_up(); }
    waiting_.clear();
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
