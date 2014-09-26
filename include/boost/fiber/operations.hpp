//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_THIS_FIBER_OPERATIONS_H
#define BOOST_THIS_FIBER_OPERATIONS_H

#include <boost/asio.hpp> 
#include <boost/chrono/system_clocks.hpp>
#include <boost/config.hpp> 
#include <boost/thread/lock_types.hpp> 

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
fibers::fiber::id get_id() BOOST_NOEXCEPT
{
    return 0 != fibers::fm_active()
        ? fibers::fm_active()->get_id()
        : fibers::fiber::id();
}

inline
void yield()
{
    if ( 0 != fibers::fm_active() )
        fibers::fm_yield();
    else
        fibers::fm_run();
}

template< typename Clock, typename Duration >
void sleep_until( chrono::time_point< Clock, Duration > const& sleep_time)
{
    if ( 0 != fibers::fm_active() )
    {
        fibers::detail::spinlock splk;
        unique_lock< fibers::detail::spinlock > lk( splk);
        fibers::fm_wait_until( sleep_time, lk);

        // check if fiber was interrupted
        interruption_point();
    }
    else
    {
        while ( chrono::high_resolution_clock::now() <= sleep_time)
            fibers::fm_run();
    }
}

template< typename Rep, typename Period >
void sleep_for( chrono::duration< Rep, Period > const& timeout_duration)
{ sleep_until( chrono::high_resolution_clock::now() + timeout_duration); }

inline
bool thread_affinity() BOOST_NOEXCEPT
{
    return 0 != fibers::fm_active()
        ? fibers::fm_active()->thread_affinity()
        : true;
}

inline
void thread_affinity( bool req) BOOST_NOEXCEPT
{
    if ( 0 != fibers::fm_active() )
        fibers::fm_active()->thread_affinity( req);
}

}

namespace fibers {

inline
void migrate( fiber const& f)
{ fm_spawn( detail::scheduler::extract( f ) ); }

inline
void set_scheduling_algorithm( sched_algorithm * al)
{ detail::scheduler::replace( al); }

template< typename Rep, typename Period >
void wait_interval( chrono::duration< Rep, Period > const& wait_interval) BOOST_NOEXCEPT
{ fm_wait_interval( wait_interval); }

template< typename Rep, typename Period >
chrono::duration< Rep, Period > wait_interval() BOOST_NOEXCEPT
{ return fm_wait_interval< Rep, Period >(); }

inline
bool preserve_fpu()
{ return fm_preserve_fpu(); }

inline
void preserve_fpu( bool preserve)
{ return fm_preserve_fpu( preserve); }

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_THIS_FIBER_OPERATIONS_H
