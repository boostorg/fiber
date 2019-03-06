
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/fiber/condition_variable.hpp"

#include "boost/fiber/context.hpp"

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

// We switch in case it's a timed-wait or a regular wait.
inline bool should_switch(fibers::context* ctx, std::intptr_t expected_tw) {
    return ctx->twstatus.compare_exchange_strong(expected_tw, static_cast<std::intptr_t>(-1),
                                                 std::memory_order_acq_rel) ||
         expected_tw == 0;
}

void
condition_variable_any::notify_one() noexcept {
    context * active_ctx = context::active();
    // get one context' from wait-queue
    detail::spinlock_lock lk{ wait_queue_splk_ };
    while ( ! wait_queue_.empty() ) {
        context * ctx = & wait_queue_.front();
        wait_queue_.pop_front();
        if (should_switch(ctx, reinterpret_cast< std::intptr_t >(this))) {
            lk.unlock();  // We can not access any data member after active_ctx->schedule() called.
            // notify context
            active_ctx->schedule( ctx);
            break;
        }
    }
}

void
condition_variable_any::notify_all() noexcept {
    context * active_ctx = context::active();

    // get all context' from wait-queue. We move the wait list into a temporary structure
    // on stack to minimize the contention on the spinlock and to prevent the scenario
    // were "this" object is destroyed but the lock still holds its spinlock member.
    detail::spinlock_lock lk{ wait_queue_splk_ };
    wait_queue_t tmp{std::move(wait_queue_)};
    lk.unlock();

    // notify all context'
    while ( ! tmp.empty() ) {
        context * ctx = & tmp.front();
        tmp.pop_front();
        if (should_switch(ctx, reinterpret_cast< std::intptr_t >( this))) {
            // notify context
            active_ctx->schedule( ctx);
        }
    }
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
