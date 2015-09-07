
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

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

class context;
struct sched_algorithm;

namespace detail {

class BOOST_FIBERS_DECL waiting_queue {
public:
    waiting_queue() noexcept :
        head_( nullptr),
        tail_( nullptr) {
    }

    waiting_queue( waiting_queue const&) = delete;
    waiting_queue & operator=( waiting_queue const&) = delete;

    bool empty() const noexcept {
        return nullptr == head_;
    }

    void push( context * item) noexcept;

    context * top() const noexcept {
        BOOST_ASSERT( ! empty() );

        return head_; 
    }

    void move_to( sched_algorithm *);

    void interrupt_all() noexcept;

    void swap( waiting_queue & other) noexcept {
        std::swap( head_, other.head_);
        std::swap( tail_, other.tail_);
    }

private:
    context   *   head_;
    context   *   tail_;
};

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DETAIL_WAITING_QUEUE_H
