
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
round_robin::awakened( context * f) {
    BOOST_ASSERT( nullptr != f);

    BOOST_ASSERT( ! f->runnable_is_linked() );
    runnable_queue_.push_back( * f);
}

context *
round_robin::pick_next() {
    context * victim( nullptr);
    if ( ! runnable_queue_.empty() ) {
        victim = & runnable_queue_.front();
        runnable_queue_.pop_front();
        BOOST_ASSERT( nullptr != victim);
        BOOST_ASSERT( ! victim->runnable_is_linked() );
    }
    return victim;
}

std::size_t
round_robin::ready_fibers() const noexcept {
    return runnable_queue_.size();
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
