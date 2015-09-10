//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_FIBER_MANAGER_H
#define BOOST_FIBERS_FIBER_MANAGER_H

#include <cstddef>
#include <chrono>
#include <memory>
#include <vector>

#include <boost/assert.hpp>
#include <boost/config.hpp>

#include <boost/fiber/context.hpp>
#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/convert.hpp>
#include <boost/fiber/detail/queues.hpp>
#include <boost/fiber/detail/spinlock.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

class context;
struct sched_algorithm;

class BOOST_FIBERS_DECL scheduler {
private:
    typedef detail::ready_queue< context >  ready_queue_t;
    typedef detail::sleep_queue< context >  sleep_queue_t;
    typedef std::vector< context * >        terminated_queue_t;

    std::unique_ptr< sched_algorithm >      sched_algo_;
    context                             *   main_context_;
    ready_queue_t                           ready_queue_;
    sleep_queue_t                           sleep_queue_;
    terminated_queue_t                      terminated_queue_;
    std::chrono::steady_clock::duration     wait_interval_;

    void resume_( context *, context *);

public:
    scheduler( context *) noexcept;

    scheduler( scheduler const&) = delete;
    scheduler & operator=( scheduler const&) = delete;

    virtual ~scheduler() noexcept;

    void spawn( context *);

    void run( context *);

    void wait( context *, detail::spinlock_lock &);

    bool wait_until( context *,
                     std::chrono::steady_clock::time_point const&);

    void yield( context *);

    void join( context *,context *);

    void signal( context *);

    size_t ready_fibers() const noexcept;

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
