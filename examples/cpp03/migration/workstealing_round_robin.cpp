
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
workstealing_round_robin::awakened( boost::fibers::detail::fiber_base * f)
{
    boost::mutex::scoped_lock lk( mtx_);
    rqueue_.push_back( f);
}

boost::fibers::detail::fiber_base *
workstealing_round_robin::pick_next()
{
    boost::mutex::scoped_lock lk( mtx_);
    boost::fibers::detail::fiber_base * f = 0;
    if ( ! rqueue_.empty() )
    {
        f = rqueue_.front();
        rqueue_.pop_front();
    }
    return f;
}

void
workstealing_round_robin::priority( boost::fibers::detail::fiber_base * f, int prio) BOOST_NOEXCEPT
{
    BOOST_ASSERT( f);

    // set only priority to fiber
    // round-robin does not respect priorities
    f->priority( prio);
}

boost::fibers::fiber
workstealing_round_robin::steal() BOOST_NOEXCEPT
{
    boost::mutex::scoped_lock lk( mtx_);
    boost::fibers::detail::fiber_base * f = 0;
    if ( ! rqueue_.empty() )
    {
        f = rqueue_.back();
        rqueue_.pop_back();
        if ( f->thread_affinity() )
        {
            rqueue_.push_back( f);
            f = 0;
        }
    }
#if 0
    if ( 0 != f)
       fprintf(stderr, "migrated fiber: %p\n", f);
#endif
    return boost::fibers::fiber( f);
}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
