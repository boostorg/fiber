
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_DETAIL_TERMINATED_QUEUE_H
#define BOOST_FIBERS_DETAIL_TERMINATED_QUEUE_H

#include <algorithm>
#include <cstddef>

#include <boost/config.hpp>

#include <boost/fiber/detail/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

class fiber_context;

namespace detail {

class BOOST_FIBERS_DECL terminated_queue {
public:
    terminated_queue() noexcept :
        head_( nullptr),
        tail_( & head_) {
    }

    ~terminated_queue() {
        clear();
    }

    terminated_queue( terminated_queue const&) = delete;
    terminated_queue & operator=( terminated_queue const&) = delete;

    void push( fiber_context * item) noexcept;

    void clear() noexcept;

    void swap( terminated_queue & other) {
        std::swap( head_, other.head_);
        std::swap( tail_, other.tail_);
    }

private:
    fiber_context   *   head_;
    // tail_ points to the nxt field that contains the null that marks the end
    // of the terminated_queue. When the terminated_queue is empty, tail_ points to head_. tail_ must
    // never be null: it always points to a real fiber_context*. However, in
    // normal use, (*tail_) is always null.
    fiber_context   **  tail_;
};

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DETAIL_TERMINATED_QUEUE_H
