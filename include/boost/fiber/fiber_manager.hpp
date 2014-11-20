//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_FIBER_MANAGER_H
#define BOOST_FIBERS_FIBER_MANAGER_H

#include <boost/assert.hpp>
#include <boost/chrono/system_clocks.hpp>
#include <boost/config.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/thread/lock_types.hpp> 
#include <boost/utility.hpp>

#include <boost/fiber/algorithm.hpp>
#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/convert.hpp>
#include <boost/fiber/detail/main_fiber.hpp>
#include <boost/fiber/detail/spinlock.hpp>
#include <boost/fiber/detail/waiting_queue.hpp>
#include <boost/fiber/detail/worker_fiber.hpp>
#include <boost/fiber/fiber.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

# if defined(BOOST_MSVC)
# pragma warning(push)
# pragma warning(disable:4251 4275)
# endif

namespace boost {
namespace fibers {

struct fiber_manager : private noncopyable
{
    fiber_manager() BOOST_NOEXCEPT;

    virtual ~fiber_manager() BOOST_NOEXCEPT;

    typedef detail::waiting_queue   wqueue_t;

    scoped_ptr< sched_algorithm >   def_algo_;
    sched_algorithm             *   sched_algo_;
    wqueue_t                        wqueue_;

    chrono::high_resolution_clock::duration            wait_interval_;
    detail::worker_fiber        *   active_fiber_;
};

void fm_resume_( detail::worker_fiber *);

void fm_set_sched_algo( sched_algorithm *);

void fm_spawn( detail::worker_fiber *);

void fm_priority( detail::worker_fiber *, int) BOOST_NOEXCEPT;

void fm_wait_interval( chrono::high_resolution_clock::duration const&) BOOST_NOEXCEPT;
template< typename Rep, typename Period >
void fm_wait_interval( chrono::duration< Rep, Period > const& wait_interval) BOOST_NOEXCEPT
{ fm_wait_interval( wait_interval); }

chrono::high_resolution_clock::duration fm_wait_interval() BOOST_NOEXCEPT;

void fm_join( detail::worker_fiber *);

detail::worker_fiber * fm_active() BOOST_NOEXCEPT;

void fm_run();

void fm_wait( unique_lock< detail::spinlock > &);

bool fm_wait_until( chrono::high_resolution_clock::time_point const&,
                    unique_lock< detail::spinlock > &);

template< typename Clock, typename Duration >
bool fm_wait_until( chrono::time_point< Clock, Duration > const& timeout_time_,
                    unique_lock< detail::spinlock > & lk)
{
    chrono::high_resolution_clock::time_point timeout_time(
            detail::convert_tp( timeout_time_) );
    return fm_wait_until( timeout_time, lk);
}

template< typename Rep, typename Period >
bool fm_wait_for( chrono::duration< Rep, Period > const& timeout_duration,
                  unique_lock< detail::spinlock > & lk)
{
    return wait_until( chrono::high_resolution_clock::now() + timeout_duration, lk);
}

void fm_yield();

chrono::high_resolution_clock::time_point fm_next_wakeup();

void fm_migrate( detail::worker_fiber *);

}}

# if defined(BOOST_MSVC)
# pragma warning(pop)
# endif

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_FIBER_MANAGER_H
