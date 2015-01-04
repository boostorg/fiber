
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_DETAIL_WAITING_QUEUE_H
#define BOOST_FIBERS_DETAIL_WAITING_QUEUE_H

#include <algorithm>

#include <boost/assert.hpp>
#include <boost/config.hpp>

#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/fiber_handle.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

struct sched_algorithm;

namespace detail {

class waiting_queue {
public:
    waiting_queue() noexcept :
        head_() {
    }

    waiting_queue( waiting_queue const&) = delete;
    waiting_queue & operator=( waiting_queue const&) = delete;

    bool empty() const noexcept {
        return ! head_;
    }

    void push( fiber_handle & item) noexcept;

    fiber_handle top() const noexcept {
        BOOST_ASSERT( ! empty() );

        return head_; 
    }

    void move_to( sched_algorithm *);

    void swap( waiting_queue & other) noexcept {
        std::swap( head_, other.head_);
    }

private:
    fiber_handle    head_;
};

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DETAIL_WAITING_QUEUE_H
