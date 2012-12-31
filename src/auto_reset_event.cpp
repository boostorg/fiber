
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
    waiting_()
{}

void
auto_reset_event::wait()
{
    while ( SET != state_)
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
    state_ = RESET;
}

bool
auto_reset_event::timed_wait( chrono::system_clock::time_point const& abs_time)
{
    while ( SET != state_)
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
    state_ = RESET;
    return chrono::system_clock::now() <= abs_time;
}

bool
auto_reset_event::try_wait()
{
    if ( SET == state_)
    {
        state_ = RESET;
        return true;
    }
    return false;
}

void
auto_reset_event::set()
{
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
    state_ = SET;
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
