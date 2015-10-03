//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_DEFAULT_ROUND_ROBIN_H
#define BOOST_FIBERS_DEFAULT_ROUND_ROBIN_H

#include <chrono>

#include <boost/config.hpp>

#include <boost/fiber/algorithm.hpp>
#include <boost/fiber/context.hpp>
#include <boost/fiber/detail/autoreset_event.hpp>
#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/scheduler.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

class context;

class BOOST_FIBERS_DECL round_robin : public sched_algorithm {
private:
    scheduler::ready_queue_t    ready_queue_;
    detail::autoreset_event     ev_;

public:
    virtual void awakened( context *);

    virtual context * pick_next();

    virtual bool has_ready_fibers() const noexcept;

    virtual void suspend_until( std::chrono::steady_clock::time_point const&);

    virtual void notify();
};

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DEFAULT_ROUND_ROBIN_H
