
//          Copyright Oliver Kowalke 2015.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_FIBERS_DETAIL_CONTEXT_WAIT_QUEUE_H
#define BOOST_FIBERS_DETAIL_CONTEXT_WAIT_QUEUE_H

#include <cstddef>
#include <cstring>
#include <mutex>

#include <boost/config.hpp>

#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/spinlock.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

class context;

namespace detail {

class context_wait_queue {
private:
    typedef context *   slot_type;

    std::size_t         pidx_{ 0 };
    std::size_t         cidx_{ 0 };
    std::size_t         capacity_;
    slot_type       *   slots_;

    void resize_() {
        slot_type * old_slots = slots_;
        slots_ = new slot_type[2*capacity_];
        std::size_t cidx = cidx_ % capacity_;
        std::size_t offset = capacity_ - cidx;
        std::memcpy( slots_, old_slots + cidx, offset * sizeof( slot_type) );
        if ( 0 < cidx) {
            std::size_t pidx = pidx_ % capacity_;
            std::memcpy( slots_ + offset, old_slots, pidx * sizeof( slot_type) );
        }
        cidx_ = 0;
        pidx_ = capacity_ - 1;
        capacity_ *= 2;
        delete [] old_slots;
    }

    bool is_full_() const noexcept {
        return (cidx_ % capacity_) == ((pidx_ + 1) % capacity_);
    }

    bool is_empty_() const noexcept {
        return cidx_ == pidx_;
    }

public:
    context_wait_queue( std::size_t capacity = 4096) :
            capacity_{ capacity } {
        slots_ = new slot_type[capacity_];
    }

    ~context_wait_queue() {
        delete [] slots_;
        slots_ = nullptr;
    }

    context_wait_queue( context_wait_queue const&) = delete;
    context_wait_queue & operator=( context_wait_queue const&) = delete;

    bool empty() const noexcept {
        return is_empty_();
    }

    void push( context * c) {
        if ( is_full_() ) {
            resize_();
        }
        slots_[pidx_ % capacity_] = c;
        ++pidx_;
    }

    context * pop() {
        context * c = nullptr;
        if ( ! is_empty_() ) {
            c = slots_[cidx_ % capacity_];
            ++cidx_;
        }
        return c;
    }

    void remove( context * c) noexcept {
        for ( std::size_t i = cidx_; i < pidx_; ++i) {
            if ( slots_[i % capacity_] == c) {
                if ( 1 < pidx_ - i) {
                    std::memcpy( slots_ + i, slots_ + i + 1, (pidx_ - i - 1) * sizeof( slot_type) );
                }
                --pidx_;
                return;
            }
        }
    }
};

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DETAIL_CONTEXT_WAIT_QUEUE_H
