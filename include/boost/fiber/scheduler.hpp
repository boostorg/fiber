//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_FIBER_MANAGER_H
#define BOOST_FIBERS_FIBER_MANAGER_H

#include <cstddef>
#include <chrono>
#include <memory>
#include <mutex>

#include <boost/assert.hpp>
#include <boost/config.hpp>

#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/convert.hpp>
#include <boost/fiber/detail/spinlock.hpp>
#include <boost/fiber/detail/terminated_queue.hpp>
#include <boost/fiber/detail/waiting_queue.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

class context;
struct sched_algorithm;

struct BOOST_FIBERS_DECL scheduler {
private:
    typedef detail::waiting_queue           wqueue_t;
    typedef detail::terminated_queue        tqueue_t;

    std::unique_ptr< sched_algorithm >      sched_algo_;
    context                             *   main_context_;
    wqueue_t                                wqueue_;
    tqueue_t                                tqueue_;
    std::chrono::steady_clock::duration     wait_interval_;

    void resume_( context *);

    bool wait_until_( std::chrono::steady_clock::time_point const&,
                      detail::spinlock_lock &);

public:
    scheduler() noexcept;

    scheduler( scheduler const&) = delete;
    scheduler & operator=( scheduler const&) = delete;

    virtual ~scheduler() noexcept;

    void spawn( context *);

    void run();

    void wait( detail::spinlock_lock &);

    template< typename Clock, typename Duration >
    bool wait_until( std::chrono::time_point< Clock, Duration > const& timeout_time_,
                     detail::spinlock_lock & lk) {
        std::chrono::steady_clock::time_point timeout_time(
                detail::convert_tp( timeout_time_) );
        return wait_until_( timeout_time, lk);
    }

    template< typename Rep, typename Period >
    bool wait_for( std::chrono::duration< Rep, Period > const& timeout_duration,
                   detail::spinlock_lock & lk) {
        return wait_until_(
                std::chrono::steady_clock::now() + timeout_duration, lk);
    }

    void yield();

    void join( context *);

    std::size_t ready_fibers() const noexcept;

    void set_sched_algo( std::unique_ptr< sched_algorithm >);

    void wait_interval( std::chrono::steady_clock::duration const&) noexcept;

    template< typename Rep, typename Period >
    void wait_interval( std::chrono::duration< Rep, Period > const& wait_interval) noexcept {
        wait_interval( wait_interval);
    }

    std::chrono::steady_clock::duration wait_interval() noexcept;
};

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_FIBER_MANAGER_H
