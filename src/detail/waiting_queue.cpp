
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

#include "boost/fiber/algorithm.hpp"
#include "boost/fiber/detail/config.hpp"
#include "boost/fiber/context.hpp"

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace detail {

void
waiting_queue::push( context * item) noexcept {
    BOOST_ASSERT( nullptr != item);
    BOOST_ASSERT( nullptr == item->nxt);

    if ( nullptr == head_) {
        head_ = tail_ = item;
    } else {
        tail_->nxt = item;
        tail_ = tail_->nxt;
    }
}

void
waiting_queue::move_to( sched_algorithm * sched_algo) {
    BOOST_ASSERT( nullptr != sched_algo);

    std::chrono::steady_clock::time_point now(
        std::chrono::steady_clock::now() );

    context * prev = head_;
    for ( context * f( head_); nullptr != f;) {
        BOOST_ASSERT( ! f->is_running() );
        BOOST_ASSERT( ! f->is_terminated() );

        // set fiber to state_ready if deadline was reached
        // set fiber to state_ready if interruption was requested
        if ( f->time_point() <= now || f->interruption_requested() ) {
            f->set_ready();
        }

        if ( ! f->is_ready() ) {
            prev = f;
            f = f->nxt;
        } else {
            if ( head_ == f) {
                head_ = f->nxt;
                prev = head_;
                context * item = f;
                f = head_;
                if ( nullptr == head_) {
                    tail_ = head_;
                }
                item->nxt = nullptr;
                // Pass the newly-unlinked context* to sched_algo.
                item->time_point_reset();
                sched_algo->awakened( item);
            } else {
                prev->nxt = f->nxt;
                if ( nullptr == prev->nxt) {
                    tail_ = prev;
                }
                context * item = f;
                f = f->nxt;
                item->nxt = nullptr;
                // Pass the newly-unlinked context* to sched_algo.
                item->time_point_reset();
                sched_algo->awakened( item);
            }
        }
    }
}

void
waiting_queue::interrupt_all() noexcept {
    for ( context * f( head_); nullptr != f; f = f->nxt) {
        f->request_interruption( true);
    }
}

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
