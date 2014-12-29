
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/fiber/detail/waiting_queue.hpp"

#include <algorithm>
#include <cstddef>
#include <chrono>

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/intrusive_ptr.hpp>

#include <boost/fiber/algorithm.hpp>
#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/fiber_base.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace detail {

void
waiting_queue::push( fiber_handle & item) noexcept {
    BOOST_ASSERT( item);
    BOOST_ASSERT( ! item->nxt);

    // Skip past any worker_fibers in the queue whose time_point() is less
    // than item->time_point(), looking for the first worker_fiber in the
    // queue whose time_point() is at least item->time_point(). Insert
    // item before that. In other words, insert item so as to preserve
    // ascending order of time_point() values. (Recall that a worker_fiber
    // waiting with no timeout uses the maximum time_point value.)

    // We do this by walking the linked list of nxt fields with a
    // worker_fiber**. In other words, first we point to &head_, then to
    // &head_->nxt then to &head_->nxt->nxt and so forth. When we find
    // the item with the right time_point(), we're already pointing to the
    // worker_fiber* that links it into the list. Insert item right there.

    fiber_handle * f = & head_;
    for ( ; * f; f = & ( * f)->nxt) {
        if ( item->time_point() <= ( * f)->time_point() ) {
            break;
        }
    }

    // Here, either we reached the end of the list (! *f) or we found a
    // (*f) before which to insert 'item'. Break the link at *f and insert
    // item.
    item->nxt = * f;
    * f = item;
}

void
waiting_queue::move_to( sched_algorithm * sched_algo) {
    BOOST_ASSERT( nullptr != sched_algo);

    std::chrono::high_resolution_clock::time_point now(
        std::chrono::high_resolution_clock::now() );

    // Search the queue for every worker_fiber 'f' for which fn(f, now)
    // returns true. Each time we find such a worker_fiber, unlink it from
    // the queue and pass it to sched_algo->awakened().

    // Search using a worker_fiber**, starting at &head_.
    for ( fiber_handle * fp = & head_; * fp;) {
        fiber_handle f = * fp;
        BOOST_ASSERT( ! f->is_running() );
        BOOST_ASSERT( ! f->is_terminated() );

        // set fiber to state_ready if dead-line was reached
        // set fiber to state_ready if interruption was requested
        if ( f->time_point() <= now || f->interruption_requested() ) {
            f->set_ready();
        }

        if ( ! f->is_ready() ) {
            // If f does NOT meet caller's criteria, skip fp past it.
            fp = & ( * fp)->nxt;
        } else {
            // Here f satisfies our caller. Unlink it from the list.
            * fp = ( * fp)->nxt;
            f->nxt.reset();
            // Pass the newly-unlinked worker_fiber* to sched_algo.
            f->time_point_reset();
            sched_algo->awakened( f);
        }
    }
}

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
