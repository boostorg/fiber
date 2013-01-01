
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
    waiting_mtx_(),
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
    if ( 0 == current_) return; //FIXME: set to initial_ instead?
    if ( 0 == --current_)
    {
        detail::spin_mutex::scoped_lock lk( waiting_mtx_);
        BOOST_FOREACH( detail::fiber_base::ptr_t const& f, waiting_)
        { f->set_ready(); }
        waiting_.clear();
    }
}

void
count_down_event::wait()
{
    BOOST_ASSERT( this_fiber::is_fiberized() );

    while ( 0 != current_)
    {
        detail::spin_mutex::scoped_lock lk( waiting_mtx_);
        waiting_.push_back(
            detail::scheduler::instance().active() );
        detail::scheduler::instance().wait( lk);
    }
}

bool
count_down_event::timed_wait( chrono::system_clock::time_point const& abs_time)
{
    BOOST_ASSERT( this_fiber::is_fiberized() );

    if ( chrono::system_clock::now() >= abs_time) return false;

    while ( 0 != current_)
    {
        detail::spin_mutex::scoped_lock lk( waiting_mtx_);
        waiting_.push_back(
            detail::scheduler::instance().active() );
        detail::scheduler::instance().wait( lk);

        if ( chrono::system_clock::now() >= abs_time) return false;
    }
    return true;
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
