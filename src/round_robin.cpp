
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/fiber/round_robin.hpp"

#include <boost/assert.hpp>

#include <boost/fiber/fiber_context.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

void
round_robin::awakened( fiber_handle & f) {
    BOOST_ASSERT( f);

    rqueue_.push( f);
}

fiber_handle
round_robin::pick_next() {
    fiber_handle victim;
    if ( ! rqueue_.empty() ) {
        victim = rqueue_.pop();
    }
    return victim;
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
