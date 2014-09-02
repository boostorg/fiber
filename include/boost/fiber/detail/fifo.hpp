
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

    void push( worker_fiber * item) BOOST_NOEXCEPT
    {
        BOOST_ASSERT(item->is_ready() || item->is_running());
        BOOST_ASSERT( 0 != item);
        BOOST_ASSERT( 0 == item->next() );
        BOOST_ASSERT( 0 == item->prev() );

        if ( empty() )
            head_ = tail_ = item;
        else
        {
            tail_->next( item);
            item->prev(tail_);
            tail_ = item;
        }
    }

    worker_fiber * pop() BOOST_NOEXCEPT
    {
        BOOST_ASSERT( ! empty() );

        worker_fiber * item = head_;
        head_ = head_->next();
        if ( 0 == head_) tail_ = 0;
        else
        {
            head_->prev(0);
        }
        item->next_reset();
        item->prev_reset();
        return item;
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

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DETAIL_FIFO_H
