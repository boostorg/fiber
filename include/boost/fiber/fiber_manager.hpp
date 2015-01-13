//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_FIBER_MANAGER_H
#define BOOST_FIBERS_FIBER_MANAGER_H

#include <chrono>
#include <mutex>

#include <boost/assert.hpp>
#include <boost/config.hpp>

#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/convert.hpp>
#include <boost/fiber/detail/spinlock.hpp>
#include <boost/fiber/detail/waiting_queue.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

class fiber_context;
struct sched_algorithm;

struct fiber_manager {
    fiber_manager() noexcept;

    fiber_manager( fiber_manager const&) = delete;
    fiber_manager & operator=( fiber_manager const&) = delete;

    virtual ~fiber_manager() noexcept;

    typedef detail::waiting_queue                   wqueue_t;

    sched_algorithm                             *   sched_algo;
    fiber_context                               *   active_fiber;
    wqueue_t                                        wqueue;
    bool                                            preserve_fpu;
    std::chrono::high_resolution_clock::duration    wait_interval;
};

void fm_resume_( fiber_context *);

std::chrono::high_resolution_clock::time_point fm_next_wakeup();

void fm_spawn( fiber_context *);

void fm_run();

void fm_wait( std::unique_lock< detail::spinlock > &);

bool fm_wait_until( std::chrono::high_resolution_clock::time_point const&,
                    std::unique_lock< detail::spinlock > &);

template< typename Clock, typename Duration >
bool fm_wait_until( std::chrono::time_point< Clock, Duration > const& timeout_time_,
                    std::unique_lock< detail::spinlock > & lk) {
    std::chrono::high_resolution_clock::time_point timeout_time(
            detail::convert_tp( timeout_time_) );
    return fm_wait_until( timeout_time, lk);
}

template< typename Rep, typename Period >
bool fm_wait_for( std::chrono::duration< Rep, Period > const& timeout_duration,
                  std::unique_lock< detail::spinlock > & lk) {
    return wait_until( std::chrono::high_resolution_clock::now() + timeout_duration, lk);
}

void fm_yield();

void fm_join( fiber_context *);

fiber_context * fm_active() noexcept;

void fm_set_sched_algo( sched_algorithm *);

void fm_wait_interval( std::chrono::high_resolution_clock::duration const&) noexcept;

template< typename Rep, typename Period >
void fm_wait_interval( std::chrono::duration< Rep, Period > const& wait_interval) noexcept {
    fm_wait_interval( wait_interval);
}

std::chrono::high_resolution_clock::duration fm_wait_interval() noexcept;

bool fm_preserve_fpu();

void fm_preserve_fpu( bool);

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_FIBER_MANAGER_H
