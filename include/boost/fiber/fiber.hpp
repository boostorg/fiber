
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_FIBER_H
#define BOOST_FIBERS_FIBER_H

#include <algorithm> // std::move()
#include <exception> // std::terminate()
#include <memory> // std::allocator_arg
#include <utility> // std::forward()

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/intrusive_ptr.hpp>

#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/fiber_handle.hpp>
#include <boost/fiber/detail/fiber_base.hpp>
#include <boost/fiber/fixedsize.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace detail {

struct scheduler;

}

class BOOST_FIBERS_DECL fiber {
private:
    friend struct detail::scheduler;

    typedef detail::fiber_handle        ptr_t;

    ptr_t                               impl_;

    void start_();

public:
    typedef detail::fiber_base::id    id;

    fiber() noexcept :
        impl_() {
    }

    explicit fiber( ptr_t impl) noexcept :
        impl_( impl) {
    }

    template< typename Fn, typename ... Args >
    explicit fiber( Fn && fn, Args && ... args) :
        fiber( std::allocator_arg, fixedsize(), std::forward< Fn >( fn), std::forward< Args >( args) ... ) {
    }

    template< typename StackAllocator, typename Fn, typename ... Args >
    explicit fiber( std::allocator_arg_t, StackAllocator salloc, Fn && fn, Args && ... args) :
        impl_( new detail::fiber_base( salloc, std::forward< Fn >( fn), std::forward< Args >( args) ... ) ) {
        start_();
    }

    ~fiber() {
        if ( joinable() ) {
            std::terminate();
        }
    }

    fiber( fiber const&) = delete;
    fiber & operator=( fiber const&) = delete;

    fiber( fiber && other) noexcept :
        impl_( std::move( other.impl_) ) {
    }

    fiber & operator=( fiber && other) noexcept {
        if ( joinable() ) {
            std::terminate();
        }
        if ( this != & other) {
            impl_ = std::move( other.impl_);
        }
        return * this;
    }

    explicit operator bool() const noexcept {
        return nullptr != impl_ && ! impl_->is_terminated();
    }

    bool operator!() const noexcept {
        return nullptr == impl_ || impl_->is_terminated();
    }

    void swap( fiber & other) noexcept {
        impl_.swap( other.impl_);
    }

    bool joinable() const noexcept {
        return nullptr != impl_ /* && ! impl_->is_terminated() */;
    }

    id get_id() const noexcept {
        return nullptr != impl_ ? impl_->get_id() : id();
    }

    bool thread_affinity() const noexcept;

    void thread_affinity( bool) noexcept;

    void detach() noexcept;

    void join();

    void interrupt() noexcept;
};

inline
bool operator<( fiber const& l, fiber const& r) {
    return l.get_id() < r.get_id();
}

inline
void swap( fiber & l, fiber & r) {
    return l.swap( r);
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_FIBER_H
