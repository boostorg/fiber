//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_FIBER_MANAGER_H
#define BOOST_FIBERS_FIBER_MANAGER_H

#include <chrono>

#include <boost/config.hpp>
#include <boost/intrusive/list.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/intrusive/set.hpp>

#include <boost/fiber/context.hpp>
#include <boost/fiber/detail/autoreset_event.hpp>
#include <boost/fiber/detail/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

class BOOST_FIBERS_DECL scheduler {
private:
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

    context                 *   main_ctx_;
    intrusive_ptr< context >    dispatcher_ctx_;
    ready_queue_t               ready_queue_;
    sleep_queue_t               sleep_queue_;
    terminated_queue_t          terminated_queue_;
    bool                        shutdown_;
    detail::autoreset_event     ready_queue_ev_;

    void resume_( context *, context *);

    context * get_next_() noexcept;

    void release_terminated_();

    void woken_up_() noexcept;

public:
    scheduler() noexcept;

    scheduler( scheduler const&) = delete;
    scheduler & operator=( scheduler const&) = delete;

    virtual ~scheduler() noexcept;

    void set_main_context( context *) noexcept;

    void set_dispatcher_context( intrusive_ptr< context >) noexcept;

    void dispatch();

    void set_ready( context *) noexcept;

    void set_terminated( context *) noexcept;

    void yield( context *) noexcept;

    bool wait_until( context *, std::chrono::steady_clock::time_point const&);

    void re_schedule( context *) noexcept;
};

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_FIBER_MANAGER_H
