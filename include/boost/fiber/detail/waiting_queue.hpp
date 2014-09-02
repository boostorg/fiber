
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
        BOOST_ASSERT( 0 == item->next() );
        BOOST_ASSERT( 0 == item->prev() );

        if ( empty() )
            head_ = tail_ = item;
        else
        {
            worker_fiber * f = head_, * prev = 0;
            if (item->time_point() == (clock_type::time_point::max)())
            {
                tail_->next(item);
                item->prev(tail_);
                tail_ = item;
                return;
            }
            if (item->is_ready())
            {
                item->next(head_);
                head_->prev(item);
                head_ = item;
                return;
            }

            do
            {
                worker_fiber * nxt = f->next();
                if ( item->time_point() <= f->time_point() )
                {
                    if ( head_ == f)
                    {
                        BOOST_ASSERT( 0 == prev);

                        item->next( f);
                        f->prev(item);
                        head_ = item;
                    }
                    else
                    {
                        BOOST_ASSERT( 0 != prev);

                        item->next( f);
                        f->prev(item);
                        prev->next( item);
                        item->prev(prev);
                    }
                    break;
                }
                else if ( tail_ == f)
                {
                    BOOST_ASSERT( 0 == nxt);

                    tail_->next( item);
                    item->prev(tail_);
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

    template< typename SchedAlgo >
    void move_to_run( SchedAlgo * sched_algo, detail::worker_fiber* f)
    {
        BOOST_ASSERT(!f->is_running());
        if (f->next() == NULL && f->prev() == NULL && f != head_)
        {
            // not a waiting fiber
        }
        else
        {
            BOOST_ASSERT(!(f->prev() == NULL && f != head_));
            BOOST_ASSERT(!(f->next() == NULL && f != tail_));
            if (f == head_)
            {
                head_ = f->next();
                if (0 == head_)
                    tail_ = 0;
                else
                {
                    head_->prev(0);
                }
            }
            else
            {
                if (0 == f->next())
                {
                    BOOST_ASSERT(tail_ == f);
                    tail_ = f->prev();
                }
                else
                {
                    f->next()->prev(f->prev());
                }
                f->prev()->next(f->next());
            }
            f->next_reset();
            f->prev_reset();
            f->time_point_reset();
        }
        sched_algo->awakened(f);
    }

    template< typename SchedAlgo, typename Fn >
    void move_to( SchedAlgo * sched_algo, Fn fn)
    {
        BOOST_ASSERT( sched_algo);

        worker_fiber * f = head_, * prev = 0;
        chrono::high_resolution_clock::time_point now( chrono::high_resolution_clock::now() );
        while ( 0 != f)
        {
            worker_fiber * nxt = f->next();
            if ( fn( f, now) )
            {
            }
            else
            {
                // since the wait time is ordered, the fibers after will not be ready.
                break;
            }
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
