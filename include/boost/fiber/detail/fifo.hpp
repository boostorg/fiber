
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
        tail_( &head_)
    {}

    bool empty() const BOOST_NOEXCEPT
    { return 0 == head_; }

    void push( fiber_base * item) BOOST_NOEXCEPT
    {
        BOOST_ASSERT( 0 != item);
        BOOST_ASSERT( 0 == item->nxt_ );

        // *tail_ holds the null marking the end of the fifo. So we can extend
        // the fifo by assigning to *tail_.
        *tail_ = item;
        // Advance tail_ to point to the new end marker.
        tail_ = &item->nxt_;
    }

    fiber_base * pop() BOOST_NOEXCEPT
    {
        BOOST_ASSERT( ! empty() );

        fiber_base * item = head_;
        head_ = head_->nxt_;
        if ( 0 == head_) tail_ = &head_;
        item->nxt_ = 0;
        return item;
    }

    void swap( fifo & other)
    {
        std::swap( head_, other.head_);
        std::swap( tail_, other.tail_);
    }

private:
    // head_ points to the head item, or is null
    fiber_base    *  head_;
    // tail_ points to the nxt_ field that contains the null that marks the
    // end of the fifo. When the fifo is empty, tail_ points to head_. tail_
    // must never be null: it always points to a real fiber_base*. However, in
    // normal use, (*tail_) is always null.
    fiber_base   **  tail_;
};

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DETAIL_FIFO_H
