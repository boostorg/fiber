
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_DETAIL_WAITING_QUEUE_H
#define BOOST_FIBERS_DETAIL_WAITING_QUEUE_H

#include <algorithm>
#include <cstddef>

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/utility.hpp>

#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/fiber_base.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace detail {

class waiting_queue : private noncopyable
{
public:
    waiting_queue() BOOST_NOEXCEPT :
        head_( 0)
    {}

    bool empty() const BOOST_NOEXCEPT
    { return 0 == head_; }

    void push( fiber_base * item) BOOST_NOEXCEPT
    {
        BOOST_ASSERT( 0 != item);
        BOOST_ASSERT( 0 == item->nxt);

        // Skip past any worker_fibers in the queue whose time_point() is less
        // than item->time_point(), looking for the first worker_fiber in the
        // queue whose time_point() is at least item->time_point(). Insert
        // item before that. In other words, insert item so as to preserve
        // ascending order of time_point() values. (Recall that a worker_fiber
        // waiting with no timeout uses the maximum time_point value.)

        // We do this by walking the linked list of nxt fields with a
        // worker_fiber**. In other words, first we point to &head_, then to
        // &head_->nxt then to &head_->nxt->nxt and so forth. When we find
        // the item with the right time_point(), we're already pointing to the
        // worker_fiber* that links it into the list. Insert item right there.

        fiber_base ** f = & head_;
        for ( ; 0 != * f; f = & ( * f)->nxt)
            if ( item->time_point() <= ( * f)->time_point() )
                break;

        // Here, either we reached the end of the list (! *f) or we found a
        // (*f) before which to insert 'item'. Break the link at *f and insert
        // item.
        item->nxt = * f;
        * f = item;
    }

    fiber_base * top() const BOOST_NOEXCEPT
    {
        BOOST_ASSERT( ! empty() );

        return head_; 
    }

    template< typename SchedAlgo, typename Fn >
    void move_to( SchedAlgo * sched_algo, Fn fn)
    {
        BOOST_ASSERT( sched_algo);

        chrono::high_resolution_clock::time_point now( chrono::high_resolution_clock::now() );

        // Search the queue for every worker_fiber 'f' for which fn(f, now)
        // returns true. Each time we find such a worker_fiber, unlink it from
        // the queue and pass it to sched_algo->awakened().

        // Search using a worker_fiber**, starting at &head_.
        for ( fiber_base ** fp = & head_; 0 != *fp;)
        {
            fiber_base * f = * fp;

            if ( ! fn( f, now) )
            {
                // If f does NOT meet caller's criteria, skip fp past it.
                fp = & ( * fp)->nxt;
            }
            else
            {
                // Here f satisfies our caller. Unlink it from the list.
                * fp = ( * fp)->nxt;
                f->nxt = 0;
                // Pass the newly-unlinked worker_fiber* to sched_algo.
                f->time_point_reset();
                sched_algo->awakened( f);
            }
        }
    }

    void swap( waiting_queue & other)
    { std::swap( head_, other.head_); }

private:
    fiber_base    *  head_;
};

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DETAIL_WAITING_QUEUE_H
