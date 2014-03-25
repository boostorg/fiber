
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
workstealing_round_robin::awakened( boost::fibers::detail::worker_fiber * f)
{ rqueue_.push( f); }

boost::fibers::detail::worker_fiber *
workstealing_round_robin::pick_next()
{ return rqueue_.pop(); }

void
workstealing_round_robin::priority( boost::fibers::detail::worker_fiber * f, int prio) BOOST_NOEXCEPT
{
    BOOST_ASSERT( f);

    // set only priority to fiber
    // round-robin does not respect priorities
    f->priority( prio);
}

boost::fibers::fiber
workstealing_round_robin::steal() BOOST_NOEXCEPT
{ return boost::fibers::fiber( rqueue_.steal() ); }

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
