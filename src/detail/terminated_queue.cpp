
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/fiber/detail/terminated_queue.hpp"

#include <boost/assert.hpp>
#include <boost/config.hpp>

#include <boost/fiber/fiber_context.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace detail {

void
terminated_queue::push( fiber_context * item) noexcept {
    BOOST_ASSERT( nullptr != item);
    BOOST_ASSERT( nullptr == item->nxt);

    // * tail_ holds the null marking the end of the terminated_queue. So we can extend
    // the terminated_queue by assigning to * tail_.
    * tail_ = item;
    // Advance tail_ to point to the new end marker.
    tail_ = & item->nxt;
}

void
terminated_queue::clear() noexcept {
    while ( nullptr != head_) {
        fiber_context * item( head_);
        head_ = head_->nxt;
        if ( nullptr == head_) {
            tail_ = & head_;
        }
        // should call ~fiber_context()
        intrusive_ptr_release( item);
    }
}

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
