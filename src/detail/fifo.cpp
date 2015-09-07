
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/fiber/detail/fifo.hpp"

#include <boost/assert.hpp>

#include <boost/fiber/context.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace detail {

void
fifo::push( context * item) noexcept {
    BOOST_ASSERT( nullptr != item);
    BOOST_ASSERT( nullptr == item->nxt);

    // * tail_ holds the null marking the end of the fifo. So we can extend
    // the fifo by assigning to * tail_.
    * tail_ = item;
    // Advance tail_ to point to the new end marker.
    tail_ = & item->nxt;
    ++size_;
}

context *
fifo::pop() noexcept {
    BOOST_ASSERT( ! empty() );

    context * item( head_);
    head_ = head_->nxt;
    if ( nullptr == head_) {
        tail_ = & head_;
    }
    item->nxt = nullptr;
    --size_;
    return item;
}

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
