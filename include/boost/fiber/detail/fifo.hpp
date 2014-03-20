
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_DETAIL_FIFO_H
#define BOOST_FIBERS_DETAIL_FIFO_H

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

class fifo : private noncopyable
{
public:
    fifo() BOOST_NOEXCEPT :
        head_( 0),
        tail_( 0)
    {}

    bool empty() const BOOST_NOEXCEPT
    { return 0 == head_; }

    std::size_t size() const BOOST_NOEXCEPT
    {
        std::size_t counter = 0; 
        for ( worker_fiber * x = head_; x; x = x->next() )
            ++counter;
        return counter;
    }

    void push( worker_fiber * item) BOOST_NOEXCEPT
    {
        BOOST_ASSERT( 0 != item);

        if ( empty() )
        {
            head_ = tail_ = item;
            return;
        }
        tail_->next( item);
        tail_ = tail_->next();
    }

    worker_fiber * head() const BOOST_NOEXCEPT
    { return head_; }

    void top( worker_fiber * item) BOOST_NOEXCEPT
    { head_ = item; }

    worker_fiber * tail() const BOOST_NOEXCEPT
    { return tail_; }

    void tail( worker_fiber * item) BOOST_NOEXCEPT
    { tail_ = item; }

    worker_fiber * pop() BOOST_NOEXCEPT
    {
        BOOST_ASSERT( ! empty() );

        worker_fiber * item = head_;
        head_ = head_->next();
        if ( ! head_)
            tail_ = head_;
        else
            item->next_reset();
        return item;
    }

    worker_fiber * find( worker_fiber * item) BOOST_NOEXCEPT
    {
        BOOST_ASSERT( 0 != item);

        for ( worker_fiber * x = head_; x; x = x->next() )
            if ( item == x) return x;
        return 0;
    }

    void erase( worker_fiber * item) BOOST_NOEXCEPT
    {
        BOOST_ASSERT( item);
        BOOST_ASSERT( ! empty() );

        if ( item == head_)
        {
            pop();
            return;
        }
        for ( worker_fiber * x = head_; x; x = x->next() )
        {
            worker_fiber * nxt = x->next();
            if ( ! nxt) return;
            if ( item == nxt)
            {
                if ( tail_ == nxt) tail_ = x;
                x->next( nxt->next() );
                nxt->next_reset();
                return;
            }
        }
    }

    template< typename SchedAlgo, typename Fn >
    void move_to( SchedAlgo * sched_algo, Fn fn)
    {
        BOOST_ASSERT( sched_algo);

        for ( worker_fiber * f = head_, * prev = head_; f; )
        {
            worker_fiber * nxt = f->next();
            if ( fn( f) )
            {
                if ( f == head_)
                {
                    head_ = nxt;
                    if ( ! head_)
                        tail_ = head_;
                    else
                    {
                        head_ = nxt;
                        prev = nxt;
                    }
                }
                else
                {
                    if ( ! nxt)
                        tail_ = prev;

                    prev->next( nxt); 
                }
                f->next_reset();
                sched_algo->awakened( f);
            }
            else
                prev = f;
            f = nxt;
        }
    }

    void swap( fifo & other)
    {
        std::swap( head_, other.head_);
        std::swap( tail_, other.tail_);
    }

private:
    worker_fiber    *  head_;
    worker_fiber    *  tail_;
};

}}}

# if defined(BOOST_MSVC)
# pragma warning(pop)
# endif

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DETAIL_FIFO_H
