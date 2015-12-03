
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_PROMISE_HPP
#define BOOST_FIBERS_PROMISE_HPP

#include <algorithm>
#include <memory>
#include <utility>

#include <boost/config.hpp>

#include <boost/fiber/exceptions.hpp>
#include <boost/fiber/future/detail/shared_state.hpp>
#include <boost/fiber/future/detail/shared_state_object.hpp>
#include <boost/fiber/future/future.hpp>

namespace boost {
namespace fibers {

template< typename R >
class promise {
private:
    typedef typename detail::shared_state< R >::ptr_t   ptr_t;

    bool            obtained_{ false };
    ptr_t           future_{};

public:
    promise() {
        typedef detail::shared_state_object<
            R, std::allocator< promise< R > >
        >                                               object_t;
        std::allocator< promise< R > > alloc;
        typename object_t::allocator_t a{ alloc };
        future_ = ptr_t{
            // placement new
            ::new( a.allocate( 1) ) object_t{ a } };
    }

    template< typename Allocator >
    promise( std::allocator_arg_t, Allocator alloc) {
        typedef detail::shared_state_object< R, Allocator >  object_t;
        typename object_t::allocator_t a{ alloc };
        future_ = ptr_t{
            // placement new
            ::new( a.allocate( 1) ) object_t{ a } };
    }

    ~promise() {
        if ( future_) {
            future_->owner_destroyed();
        }
    }

    promise( promise const&) = delete;
    promise & operator=( promise const&) = delete;

    promise( promise && other) noexcept :
        obtained_{ other.obtained_ },
        future_{ std::move( other.future_) } {
        other.obtained_ = false;
    }

    promise & operator=( promise && other) noexcept {
        if ( this == & other) return * this;
        promise tmp{ std::move( other) };
        swap( tmp);
        return * this;
    }

    void swap( promise & other) noexcept {
        std::swap( obtained_, other.obtained_);
        future_.swap( other.future_);
    }

    future< R > get_future() {
        if ( obtained_) {
            throw future_already_retrieved{};
        }
        if ( ! future_) {
            throw promise_uninitialized{};
        }
        obtained_ = true;
        return future< R >{ future_ };
    }

    void set_value( R const& value) {
        if ( ! future_) {
            throw promise_uninitialized{};
        }
        future_->set_value( value);
    }

    void set_value( R && value) {
        if ( ! future_) {
            throw promise_uninitialized{};
        }
        future_->set_value( std::move( value) );
    }

    void set_exception( std::exception_ptr p) {
        if ( ! future_) {
            throw promise_uninitialized{};
        }
        future_->set_exception( p);
    }
};

template< typename R >
class promise< R & > {
private:
    typedef typename detail::shared_state< R & >::ptr_t   ptr_t;

    bool            obtained_{ false };
    ptr_t           future_{};

public:
    promise() {
        typedef detail::shared_state_object<
            R &, std::allocator< promise< R & > >
        >                                               object_t;
        std::allocator< promise< R > > alloc{};
        typename object_t::allocator_t a{ alloc };
        future_ = ptr_t{
            // placement new
            ::new( a.allocate( 1) ) object_t{ a } };
    }

    template< typename Allocator >
    promise( std::allocator_arg_t, Allocator alloc) {
        typedef detail::shared_state_object< R &, Allocator >  object_t;
        typename object_t::allocator_t a{ alloc };
        future_ = ptr_t{
            // placement new
            ::new( a.allocate( 1) ) object_t{ a } };
    }

    ~promise() {
        if ( future_) {
            future_->owner_destroyed();
        }
    }

    promise( promise const&) = delete;
    promise & operator=( promise const&) = delete;

    promise( promise && other) noexcept :
        obtained_{ other.obtained_ },
        future_{ std::move( other.future_) } {
        other.obtained_ = false;
    }

    promise & operator=( promise && other) noexcept {
        if ( this == & other) return * this;
        promise tmp{ std::move( other) };
        swap( tmp);
        return * this;
    }

    void swap( promise & other) noexcept {
        std::swap( obtained_, other.obtained_);
        future_.swap( other.future_);
    }

    future< R & > get_future() {
        if ( obtained_) {
            throw future_already_retrieved{};
        }
        if ( ! future_) {
            throw promise_uninitialized{};
        }
        obtained_ = true;
        return future< R & >{ future_ };
    }

    void set_value( R & value) {
        if ( ! future_) {
            throw promise_uninitialized{};
        }
        future_->set_value( value);
    }

    void set_exception( std::exception_ptr p) {
        if ( ! future_) {
            throw promise_uninitialized{};
        }
        future_->set_exception( p);
    }
};

template<>
class promise< void > {
private:
    typedef detail::shared_state< void >::ptr_t   ptr_t;

    bool            obtained_{ false };
    ptr_t           future_{};

public:
    promise() {
        typedef detail::shared_state_object<
            void, std::allocator< promise< void > >
        >                                               object_t;
        std::allocator< promise< void > > alloc{};
        object_t::allocator_t a{ alloc };
        future_ = ptr_t{
            // placement new
            ::new( a.allocate( 1) ) object_t{ a } };
    }

    template< typename Allocator >
    promise( std::allocator_arg_t, Allocator alloc) {
        typedef detail::shared_state_object< void, Allocator >  object_t;
        typename object_t::allocator_t a( alloc);
        future_ = ptr_t{
            // placement new
            ::new( a.allocate( 1) ) object_t{ a } };
    }

    ~promise() {
        if ( future_) {
            future_->owner_destroyed();
        }
    }

    promise( promise const&) = delete;
    promise & operator=( promise const&) = delete;

    inline
    promise( promise && other) noexcept :
        obtained_{ other.obtained_ },
        future_{ std::move( other.future_) } {
        other.obtained_ = false;
    }

    inline
    promise & operator=( promise && other) noexcept {
        if ( this == & other) return * this;
        promise tmp{ std::move( other) };
        swap( tmp);
        return * this;
    }

    inline
    void swap( promise & other) noexcept {
        std::swap( obtained_, other.obtained_);
        future_.swap( other.future_);
    }

    inline
    future< void > get_future() {
        if ( obtained_) {
            throw future_already_retrieved{};
        }
        if ( ! future_) {
            throw promise_uninitialized{};
        }
        obtained_ = true;
        return future< void >{ future_ };
    }

    inline
    void set_value() {
        if ( ! future_) {
            throw promise_uninitialized{};
        }
        future_->set_value();
    }

    inline
    void set_exception( std::exception_ptr p) {
        if ( ! future_) {
            throw promise_uninitialized{};
        }
        future_->set_exception( p);
    }
};

template< typename R >
void swap( promise< R > & l, promise< R > & r) {
    l.swap( r);
}

}}

#endif // BOOST_FIBERS_PROMISE_HPP
