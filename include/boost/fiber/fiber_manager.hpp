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
#include <boost/fiber/detail/terminated_queue.hpp>
#include <boost/fiber/detail/waiting_queue.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

class fiber_context;
struct sched_algorithm;

struct BOOST_FIBERS_DECL fiber_manager {
private:
    typedef detail::waiting_queue                   wqueue_t;
    typedef detail::terminated_queue                tqueue_t;

    sched_algorithm                             *   sched_algo_;
    fiber_context                               *   active_fiber_;
    wqueue_t                                        wqueue_;
    tqueue_t                                        tqueue_;
    bool                                            preserve_fpu_;
    std::chrono::high_resolution_clock::duration    wait_interval_;

    void resume_( fiber_context *);

public:
    fiber_manager() noexcept;

    fiber_manager( fiber_manager const&) = delete;
    fiber_manager & operator=( fiber_manager const&) = delete;

    virtual ~fiber_manager() noexcept;

    std::chrono::high_resolution_clock::time_point next_wakeup();

    void spawn( fiber_context *);

    void run();

    void wait( std::unique_lock< detail::spinlock > &);

    bool wait_until( std::chrono::high_resolution_clock::time_point const&,
                        std::unique_lock< detail::spinlock > &);

    template< typename Clock, typename Duration >
    bool wait_until( std::chrono::time_point< Clock, Duration > const& timeout_time_,
                        std::unique_lock< detail::spinlock > & lk) {
        std::chrono::high_resolution_clock::time_point timeout_time(
                detail::convert_tp( timeout_time_) );
        return wait_until( timeout_time, lk);
    }

    template< typename Rep, typename Period >
    bool wait_for( std::chrono::duration< Rep, Period > const& timeout_duration,
                      std::unique_lock< detail::spinlock > & lk) {
        return wait_until( std::chrono::high_resolution_clock::now() + timeout_duration, lk);
    }

    void yield();

    void join( fiber_context *);

    fiber_context * active() noexcept;

    sched_algorithm* get_sched_algo_();

    void set_sched_algo( sched_algorithm *);

    void wait_interval( std::chrono::high_resolution_clock::duration const&) noexcept;

    template< typename Rep, typename Period >
    void wait_interval( std::chrono::duration< Rep, Period > const& wait_interval) noexcept {
        wait_interval( wait_interval);
    }

    std::chrono::high_resolution_clock::duration wait_interval() noexcept;

    bool preserve_fpu() const;
    void preserve_fpu( bool);
};

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_FIBER_MANAGER_H
