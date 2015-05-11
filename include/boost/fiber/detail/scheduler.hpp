//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_DETAIL_SCHEDULER_H
#define BOOST_FIBERS_DETAIL_SCHEDULER_H

#include <boost/assert.hpp>
#include <boost/config.hpp>

#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/fiber_manager.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

class fiber_context;
struct fiber_manager;
struct sched_algorithm;

namespace detail {

struct scheduler {
    scheduler( scheduler const&) = delete;
    scheduler & operator=( scheduler const&) = delete;

    template< typename F >
    static fiber_context * extract( F const& f) noexcept {
        return const_cast< F & >( f).impl_.get();
    }

    static fiber_manager * instance() noexcept {
        static thread_local fiber_manager mgr;
        return & mgr;
    }

    static void replace( sched_algorithm * other) {
        BOOST_ASSERT( nullptr != other);

        instance()->set_sched_algo( other);
    }
};

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DETAIL_SCHEDULER_H
