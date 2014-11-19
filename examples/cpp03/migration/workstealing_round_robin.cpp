
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "workstealing_round_robin.hpp"

#include <boost/assert.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

workstealing_round_robin::workstealing_round_robin():
    rhead_(0),
    rtail_(&rhead_)
{}

void
workstealing_round_robin::awakened_props( boost::fibers::fiber_base * f)
{
    boost::mutex::scoped_lock lk( mtx_);
    // append this fiber_base* to ready queue
    BOOST_ASSERT(! f->nxt_);
    *rtail_ = f;
    rtail_ = &f->nxt_;
}

boost::fibers::fiber_base *
workstealing_round_robin::pick_next()
{
    boost::mutex::scoped_lock lk( mtx_);
    boost::fibers::fiber_base * f = 0;
    if ( rhead_ )
    {
        f = rhead_;
        // pop head item from ready queue
        rhead_ = rhead_->nxt_;
        f->nxt_ = 0;
        // if that was the last item, reset tail_
        if (! rhead_)
            rtail_ = &rhead_;
    }
    return f;
}

boost::fibers::fiber
workstealing_round_robin::steal() BOOST_NOEXCEPT
{
    boost::mutex::scoped_lock lk( mtx_);

    // Search the queue for the LAST fiber_base that's willing to migrate,
    // in other words (! thread_affinity).
    boost::fibers::fiber_base ** fp = &rhead_, ** found = 0;
    for ( ; *fp; fp = &(*fp)->nxt_)
    {
        // do not consider any fiber whose thread_affinity is set
        if (! properties(*fp).thread_affinity)
            found = fp;
    }
    if (! found)
    {
        // either the queue is completely empty or all current entries have
        // thread_affinity set
        return boost::fibers::fiber(static_cast<boost::fibers::fiber_base*>(0));
    }
    // We found at least one fiber_base whose thread_affinity is NOT set;
    // *found points to the last of these. Unlink and return it.
    boost::fibers::fiber_base* ret = *found;
    *found = ret->nxt_;
    ret->nxt_ = 0;
    // if that was the last item, reset tail_
    if (! *found)
        rtail_ = &rhead_;
    return boost::fibers::fiber(ret);
}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
