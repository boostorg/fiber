
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
#include <boost/fiber/detail/fiber_base.hpp>

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
        tail_( & head_)
    {}

    bool empty() const BOOST_NOEXCEPT
    { return 0 == head_; }

    void push( fiber_base * item) BOOST_NOEXCEPT
    {
        BOOST_ASSERT( 0 != item);
        BOOST_ASSERT( 0 == item->nxt);

        // * tail_ holds the null marking the end of the fifo. So we can extend
        // the fifo by assigning to * tail_.
        * tail_ = item;
        // Advance tail_ to point to the new end marker.
        tail_ = & item->nxt;
    }

    fiber_base * pop() BOOST_NOEXCEPT
    {
        BOOST_ASSERT( ! empty() );

        fiber_base * item = head_;
        head_ = head_->nxt;
        if ( 0 == head_) tail_ = & head_;
        item->nxt = 0;
        return item;
    }

    void swap( fifo & other)
    {
        std::swap( head_, other.head_);
        std::swap( tail_, other.tail_);
    }

private:
    fiber_base    *  head_;
    fiber_base    ** tail_;
};

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DETAIL_FIFO_H
