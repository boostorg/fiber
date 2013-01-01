
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
	enter_mtx_( false),
	check_mtx_()
{}

condition::~condition()
{ BOOST_ASSERT( 0 == waiters_); }

void
condition::notify_one()
{
    BOOST_ASSERT( this_fiber::is_fiberized() );

	enter_mtx_.lock();

	if ( 0 == waiters_)
	{
		enter_mtx_.unlock();
		return;
	}

    command expected = NOTIFY_ONE;
    while ( SLEEPING != cmd_.compare_exchange_strong( expected, SLEEPING) )
    {
        this_fiber::yield();
        expected = NOTIFY_ONE;
    }
}

void
condition::notify_all()
{
    BOOST_ASSERT( this_fiber::is_fiberized() );

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
    command expected = NOTIFY_ALL;
    while ( SLEEPING != cmd_.compare_exchange_strong( expected, SLEEPING) )
    {
        this_fiber::yield();
        expected = NOTIFY_ALL;
    }
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
