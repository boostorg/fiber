
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
	check_mtx_(),
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
	
    if ( ! waiting_.empty() )
    {
        detail::fiber_base::ptr_t f;
        do
        {
            f.swap( waiting_.front() );
            waiting_.pop_front();
        } while ( f->is_complete() );
        if ( f)
            detail::scheduler::instance().notify( f);
    }
    cmd_ = NOTIFY_ONE;
}

void
condition::notify_all()
{
	enter_mtx_.lock();

	if ( 0 == waiters_)
	{
		enter_mtx_.unlock();
		return;
	}
	
    if ( ! waiting_.empty() )
    {
        BOOST_FOREACH( detail::fiber_base::ptr_t const& f,  waiting_)
        {
            if ( ! f->is_complete() )
                detail::scheduler::instance().notify( f);
        }
        waiting_.clear();
    }

    cmd_ = NOTIFY_ALL;
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
