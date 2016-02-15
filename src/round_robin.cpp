
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/fiber/round_robin.hpp"

#include <boost/assert.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

void
round_robin::awakened( context * ctx) noexcept {
    BOOST_ASSERT( nullptr != ctx);

    BOOST_ASSERT( ! ctx->ready_is_linked() );
    ctx->ready_link( ready_queue_);
}

context *
round_robin::pick_next() noexcept {
    context * victim{ nullptr };
    if ( ! ready_queue_.empty() ) {
        victim = & ready_queue_.front();
        ready_queue_.pop_front();
        BOOST_ASSERT( nullptr != victim);
        BOOST_ASSERT( ! victim->ready_is_linked() );
    }
    return victim;
}

bool
round_robin::has_ready_fibers() const noexcept {
    return ! ready_queue_.empty();
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
