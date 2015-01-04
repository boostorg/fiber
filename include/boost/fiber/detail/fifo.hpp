
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_DETAIL_FIFO_H
#define BOOST_FIBERS_DETAIL_FIFO_H

#include <algorithm>
#include <cstddef>

#include <boost/config.hpp>

#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/fiber_handle.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace detail {

class fifo {
public:
    fifo() noexcept :
        head_(),
        tail_( & head_) {
    }

    fifo( fifo const&) = delete;
    fifo & operator=( fifo const&) = delete;

    bool empty() const noexcept {
        return ! head_;
    }

    void push( fiber_handle & item) noexcept;

    fiber_handle pop() noexcept;

    void swap( fifo & other) {
        head_.swap( other.head_);
        std::swap( tail_, other.tail_);
    }

private:
    fiber_handle        head_;
    fiber_handle    *   tail_;
};

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DETAIL_FIFO_H
