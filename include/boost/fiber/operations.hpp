//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_THIS_FIBER_OPERATIONS_H
#define BOOST_THIS_FIBER_OPERATIONS_H

#include <cstddef>
#include <chrono>
#include <memory>
#include <mutex> // std::unique_lock

#include <boost/config.hpp> 
#include <boost/assert.hpp>

#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/spinlock.hpp>
#include <boost/fiber/fiber.hpp>
#include <boost/fiber/fiber_context.hpp>
#include <boost/fiber/interruption.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace this_fiber {

inline
fibers::fiber::id get_id() noexcept {
    return fibers::fiber_context::active()->get_id();
}

inline
void yield() {
    fibers::fiber_context::active()->do_yield();
}

template< typename Clock, typename Duration >
void sleep_until( std::chrono::time_point< Clock, Duration > const& sleep_time) {
    fibers::detail::spinlock splk;
    std::unique_lock< fibers::detail::spinlock > lk( splk);
    fibers::fiber_context::active()->do_wait_until( sleep_time, lk);

    // check if fiber was interrupted
    interruption_point();
}

template< typename Rep, typename Period >
void sleep_for( std::chrono::duration< Rep, Period > const& timeout_duration) {
    sleep_until( std::chrono::steady_clock::now() + timeout_duration);
}

template< typename PROPS >
PROPS & properties() {
    fibers::fiber_properties * props =
        fibers::fiber_context::active()->get_properties();
    if ( ! props) {
        // props could be nullptr if the thread's main fiber has not yet
        // yielded (not yet passed through sched_algorithm_with_properties::
        // awakened()). Address that by yielding right now.
        yield();
        // Try again to obtain the fiber_properties subclass instance ptr.
        // Walk through the whole chain again because who knows WHAT might
        // have happened while we were yielding!
        props = fibers::fiber_context::active()->get_properties();
        // Could still be hosed if the running manager isn't a subclass of
        // sched_algorithm_with_properties.
        BOOST_ASSERT_MSG(props, "this_fiber::properties not set");
    }
    return dynamic_cast< PROPS & >( * props );
}

} // this_fiber

namespace fibers {

inline
void migrate( fiber const& f) {
    fiber_context::active()->do_spawn( f);
}

template< typename SchedAlgo, typename ... Args >
void use_scheduling_algorithm( Args && ... args) {
    fiber_context::active()->do_set_sched_algo(
        std::make_unique< SchedAlgo >( std::forward< Args >( args) ... ) );
}

template< typename Rep, typename Period >
void wait_interval( std::chrono::duration< Rep, Period > const& wait_interval) noexcept {
    fiber_context::active()->do_wait_interval( wait_interval);
}

inline
std::chrono::steady_clock::duration wait_interval() noexcept {
    return fiber_context::active()->do_wait_interval();
}

template< typename Rep, typename Period >
std::chrono::duration< Rep, Period > wait_interval() noexcept {
    return fiber_context::active()->do_wait_interval< Rep, Period >();
}

inline
std::size_t ready_fibers() {
    return fiber_context::active()->do_ready_fibers();
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_THIS_FIBER_OPERATIONS_H
