
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "workstealing_round_robin.hpp"

#include <boost/assert.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

void
workstealing_round_robin::awakened( boost::fibers::fiber_context * f) {
    BOOST_ASSERT( nullptr != f);

    std::unique_lock< boost::fibers::mutex > lk( mtx_);
    BOOST_ASSERT( f->is_ready() );
    rqueue_.push_back( f);
}

boost::fibers::fiber_context *
workstealing_round_robin::pick_next() {
    std::unique_lock< boost::fibers::mutex > lk( mtx_);
    boost::fibers::fiber_context * f( nullptr);
    if ( ! rqueue_.empty() ) {
        f = rqueue_.front();
        rqueue_.pop_front();
        BOOST_ASSERT( nullptr != f);
        BOOST_ASSERT( f->is_ready() );
    }
    return f;
}

boost::fibers::fiber
workstealing_round_robin::steal() {
    std::unique_lock< boost::fibers::mutex > lk( mtx_);
    for ( boost::fibers::fiber_context * f : rqueue_) {
        BOOST_ASSERT( nullptr != f);
        if ( ! f->thread_affinity() ) {
            rqueue_.remove( f);
            BOOST_ASSERT( f->is_ready() );
            return boost::fibers::fiber( f);
        }
    }
    return boost::fibers::fiber();
}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
