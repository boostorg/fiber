//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_FIBER_MANAGER_H
#define BOOST_FIBERS_FIBER_MANAGER_H

#include <chrono>
#include <memory>

#include <boost/config.hpp>
#include <boost/intrusive/list.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/intrusive/set.hpp>

#include <boost/fiber/algorithm.hpp>
#include <boost/fiber/context.hpp>
#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/spinlock.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

class BOOST_FIBERS_DECL scheduler {
public:
    struct timepoint_less {
        bool operator()( context const& l, context const& r) {
            return l.tp_ < r.tp_;
        }
    };

    typedef intrusive::list<
                context,
                intrusive::member_hook<
                    context, detail::ready_hook, & context::ready_hook_ >,
                intrusive::constant_time_size< false > >    ready_queue_t;
    typedef intrusive::list<
                context,
                intrusive::member_hook<
                    context, detail::remote_ready_hook, & context::remote_ready_hook_ >,
                intrusive::constant_time_size< false > >    remote_ready_queue_t;
    typedef intrusive::list<
                context,
                intrusive::member_hook<
                    context, detail::yield_hook, & context::yield_hook_ >,
                intrusive::constant_time_size< false > >    yield_queue_t;
    typedef intrusive::set<
                context,
                intrusive::member_hook<
                    context, detail::sleep_hook, & context::sleep_hook_ >,
                intrusive::constant_time_size< false >,
                intrusive::compare< timepoint_less > >      sleep_queue_t;
    typedef intrusive::list<
                context,
                intrusive::member_hook<
                    context,
                    detail::terminated_hook,
                    & context::terminated_hook_ >,
                intrusive::constant_time_size< false > >    terminated_queue_t;
    typedef intrusive::list<
                context,
                intrusive::member_hook<
                    context,
                    detail::worker_hook,
                    & context::worker_hook_ >,
                intrusive::constant_time_size< false > >    worker_queue_t;

private:
    std::unique_ptr< sched_algorithm >  sched_algo_;
    context                         *   main_ctx_;
    intrusive_ptr< context >            dispatcher_ctx_;
    // worker-queue contains all context' mananged by this scheduler
    // except main-context and dispatcher-context
    // unlink happens on destruction of a context
    worker_queue_t                      worker_queue_;
    // terminated-queue contains context' which have been terminated
    terminated_queue_t                  terminated_queue_;
    // remote ready-queue contains context' signaled by schedulers
    // running in other threads
    remote_ready_queue_t                remote_ready_queue_;
    // sleep-queue cotnains context' whic hahve been called
    // scheduler::wait_until()
    yield_queue_t                       yield_queue_;
    sleep_queue_t                       sleep_queue_;
    bool                                shutdown_;
    detail::spinlock                    remote_ready_splk_;

    void resume_( context *, context *);

    context * get_next_() noexcept;

    void release_terminated_();

    void remote_ready2ready_();

    void yield2ready_();

    void sleep2ready_() noexcept;

public:
    scheduler() noexcept;

    scheduler( scheduler const&) = delete;
    scheduler & operator=( scheduler const&) = delete;

    virtual ~scheduler() noexcept;

    void set_main_context( context *) noexcept;

    void set_dispatcher_context( intrusive_ptr< context >) noexcept;

    void dispatch();

    void set_ready( context *) noexcept;

    void set_remote_ready( context *) noexcept;

    void set_terminated( context *) noexcept;

    void yield( context *) noexcept;

    bool wait_until( context *, std::chrono::steady_clock::time_point const&) noexcept;

    void re_schedule( context *) noexcept;

    bool has_ready_fibers() const noexcept;

    void set_sched_algo( std::unique_ptr< sched_algorithm >);
};

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_FIBER_MANAGER_H
