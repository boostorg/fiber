
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_FIBERS_SOURCE

#include "boost/fiber/count_down_event.hpp"

#include <boost/assert.hpp>
#include <boost/foreach.hpp>

#include <boost/fiber/detail/scheduler.hpp>
#include <boost/fiber/operations.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

count_down_event::count_down_event( std::size_t initial) :
	initial_( initial),
	current_( initial_),
    waiting_()
{}

std::size_t
count_down_event::initial() const
{ return initial_; }

std::size_t
count_down_event::current() const
{ return current_; }

bool
count_down_event::is_set() const
{ return 0 == current_; }

void
count_down_event::set()
{
	if ( 0 == current_) return;
	if ( 0 == --current_)
    {
        BOOST_FOREACH( detail::fiber_base::ptr_t const& f, waiting_)
        {
            if ( ! f->is_complete() )
                detail::scheduler::instance().notify( f);
        }
        waiting_.clear();
    }
}

void
count_down_event::wait()
{
	while ( 0 != current_)
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
}

bool
count_down_event::timed_wait( chrono::system_clock::time_point const& abs_time)
{
	while ( 0 != current_)
	{
	    if ( this_fiber::is_fiberized() )
        {
            waiting_.push_back(
                detail::scheduler::instance().active() );
            detail::scheduler::instance().sleep( abs_time);
        }
        else
            detail::scheduler::instance().run();
	}
    return chrono::system_clock::now() <= abs_time;
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
