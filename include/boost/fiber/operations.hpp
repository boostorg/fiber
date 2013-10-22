//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_THIS_FIBER_OPERATIONS_H
#define BOOST_THIS_FIBER_OPERATIONS_H

#include <boost/thread/lock_types.hpp> 

#include <boost/fiber/algorithm.hpp>
#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/scheduler.hpp>
#include <boost/fiber/detail/spinlock.hpp>
#include <boost/fiber/fiber.hpp>
#include <boost/fiber/interruption.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace this_fiber {

inline
fibers::fiber::id get_id()
{
    return fibers::detail::scheduler::instance()->active()
        ? fibers::detail::scheduler::instance()->active()->get_id()
        : fibers::fiber::id();
}

inline
void yield()
{ fibers::detail::scheduler::instance()->yield(); }

inline
void sleep_until( fibers::clock_type::time_point const& sleep_time)
{
    fibers::detail::spinlock splk;
    unique_lock< fibers::detail::spinlock > lk( splk);
    fibers::detail::scheduler::instance()->wait_until( sleep_time, lk);

    // check if fiber was interrupted
    interruption_point();
}

template< typename Rep, typename Period >
void sleep_for( chrono::duration< Rep, Period > const& timeout_duration)
{
    fibers::detail::spinlock splk;
    unique_lock< fibers::detail::spinlock > lk( splk);
    fibers::detail::scheduler::instance()->wait_for( timeout_duration, lk);

    // check if fiber was interrupted
    interruption_point();
}

}

namespace fibers {

inline
algorithm * set_scheduling_algorithm( algorithm * al)
{ return detail::scheduler::replace( al); }

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_THIS_FIBER_OPERATIONS_H
