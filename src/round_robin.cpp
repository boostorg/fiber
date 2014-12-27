
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/fiber/round_robin.hpp"

#include <boost/assert.hpp>

#include <boost/fiber/detail/fiber_base.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

void
round_robin::awakened( detail::fiber_handle f) {
    BOOST_ASSERT( f);

    rqueue_.push( f);
}

detail::fiber_handle
round_robin::pick_next() {
    detail::fiber_handle victim;
    if ( ! rqueue_.empty() ) {
        victim = rqueue_.pop();
    }
    return victim;
}

void
round_robin::priority( detail::fiber_handle f, int prio) noexcept {
    BOOST_ASSERT( f);

    // set only priority to fiber
    // round-robin does not respect priorities
    f->priority( prio);
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
