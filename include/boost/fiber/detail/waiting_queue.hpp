
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
#include <boost/fiber/detail/worker_fiber.hpp>

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
        head_( 0),
        tail_( 0)
    {}

    bool empty() const BOOST_NOEXCEPT
    { return 0 == head_; }

    void push( worker_fiber * item) BOOST_NOEXCEPT
    {
        BOOST_ASSERT( 0 != item);
        BOOST_ASSERT( 0 == item->nxt_ );

        if ( empty() )
            head_ = tail_ = item;
        else
        {
            worker_fiber * f = head_, * prev = 0;
            do
            {
                worker_fiber * nxt = static_cast<worker_fiber*>(f->nxt_);
                if ( item->time_point() <= f->time_point() )
                {
                    if ( head_ == f)
                    {
                        BOOST_ASSERT( 0 == prev);

                        item->nxt_ = f;
                        head_ = item;
                    }
                    else
                    {
                        BOOST_ASSERT( 0 != prev);

                        item->nxt_ = f;
                        prev->nxt_ = item;
                    }
                    break;
                }
                else if ( tail_ == f)
                {
                    BOOST_ASSERT( 0 == nxt);

                    tail_->nxt_ = item;
                    tail_ = item;
                    break;
                }

                prev = f;
                f = nxt;
            }
            while ( 0 != f);
        }
    }

    worker_fiber * top() const BOOST_NOEXCEPT
    {
        BOOST_ASSERT( ! empty() );

        return head_; 
    }

    template< typename SchedAlgo, typename Fn >
    void move_to( SchedAlgo * sched_algo, Fn fn)
    {
        BOOST_ASSERT( sched_algo);

        worker_fiber * f = head_, * prev = 0;
        chrono::high_resolution_clock::time_point now( chrono::high_resolution_clock::now() );
        while ( 0 != f)
        {
            worker_fiber * nxt = static_cast<worker_fiber*>(f->nxt_);
            if ( fn( f, now) )
            {
                if ( f == head_)
                {
                    BOOST_ASSERT( 0 == prev);

                    head_ = nxt;
                    if ( 0 == head_)
                        tail_ = 0;
                }
                else
                {
                    BOOST_ASSERT( 0 != prev);

                    if ( 0 == nxt)
                        tail_ = prev;

                    prev->nxt_ = nxt;
                }
                f->nxt_ = 0;
                f->time_point_reset();
                sched_algo->awakened( f);
            }
            else
                prev = f;
            f = nxt;
        }
    }

    void swap( waiting_queue & other)
    {
        std::swap( head_, other.head_);
        std::swap( tail_, other.tail_);
    }

private:
    worker_fiber    *  head_;
    worker_fiber    *  tail_;
};

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DETAIL_WAITING_QUEUE_H
