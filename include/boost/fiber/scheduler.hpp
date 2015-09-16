//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_FIBER_MANAGER_H
#define BOOST_FIBERS_FIBER_MANAGER_H

#include <boost/config.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/intrusive/list.hpp>

#include <boost/fiber/context.hpp>
#include <boost/fiber/detail/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

class BOOST_FIBERS_DECL scheduler {
private:
    typedef intrusive::list<
                context,
                intrusive::member_hook<
                    context, detail::ready_hook, & context::ready_hook_ >,
                intrusive::constant_time_size< false > >    ready_queue_t;
    typedef intrusive::list<
                context,
                intrusive::member_hook<
                    context, detail::terminated_hook, & context::terminated_hook_ >,
                intrusive::constant_time_size< false > >    terminated_queue_t;

    context                 *   main_ctx_;
    intrusive_ptr< context >    dispatcher_ctx_;
    ready_queue_t               ready_queue_;
    terminated_queue_t          terminated_queue_;
    bool                        shutdown_;

    void resume_( context *, context *);

    context * get_next_() noexcept;

    void release_terminated_();

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

    void re_schedule( context *) noexcept;
};

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_FIBER_MANAGER_H
