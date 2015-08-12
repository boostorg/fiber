
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_PACKAGED_TASK_HPP
#define BOOST_FIBERS_PACKAGED_TASK_HPP

#include <algorithm> // std::move()
#include <memory> // std::allocator_arg
#include <utility> // std::forward()

#include <boost/config.hpp>

#include <boost/fiber/exceptions.hpp>
#include <boost/fiber/future/detail/task_base.hpp>
#include <boost/fiber/future/detail/task_object.hpp>
#include <boost/fiber/future/future.hpp>

namespace boost {
namespace fibers {

template< typename Signature >
class packaged_task;

template< typename R, typename ... Args >
class packaged_task< R( Args ... ) > {
private:
    typedef typename detail::task_base< R, Args ... >::ptr_t   ptr_t;

    bool            obtained_;
    ptr_t           task_;

public:
    packaged_task() noexcept :
        obtained_( false),
        task_() {
    }

    ~packaged_task() {
        if ( task_) {
            task_->owner_destroyed();
        }
    }

    template< typename Fn >
    explicit packaged_task( Fn && fn) :
        obtained_( false),
        task_() {
        typedef detail::task_object<
            Fn,
            std::allocator< packaged_task< R() > >,
            R,
            Args ...
        >                                       object_t;
        std::allocator< packaged_task< R() > > alloc;
        typename object_t::allocator_t a( alloc);
        task_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( a, std::forward< Fn >( fn) ) );
    }

    template< typename Fn, typename Allocator >
    explicit packaged_task( std::allocator_arg_t, Allocator const& alloc, Fn && fn) :
        obtained_( false),
        task_() {
        typedef detail::task_object<
            Fn,
            Allocator,
            R
        >                                       object_t;
        typename object_t::allocator_t a( alloc);
        task_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( a, std::forward< Fn >( fn) ) );
    }

    packaged_task( packaged_task const&) = delete;
    packaged_task & operator=( packaged_task const&) = delete;

    packaged_task( packaged_task && other) noexcept :
        obtained_( other.obtained_),
        task_( std::move( other.task_) ) {
        other.obtained_ = false;
    }

    packaged_task & operator=( packaged_task && other) noexcept {
        if ( this != & other) {
            obtained_ = other.obtained_;
            other.obtained_ = false;
            task_ = std::move( other.task_);
        }
        return * this;
    }

    void swap( packaged_task & other) noexcept {
        std::swap( obtained_, other.obtained_);
        task_.swap( other.task_);
    }

    bool valid() const noexcept {
        return nullptr != task_.get();
    }

    future< R > get_future() {
        if ( obtained_) {
            throw future_already_retrieved();
        }
        if ( ! valid() ) {
            throw packaged_task_uninitialized();
        }
        obtained_ = true;
        return future< R >(
             boost::static_pointer_cast< detail::shared_state< R > >( task_) );
    }

    void operator()( Args && ... args) {
        if ( ! valid() ) {
            throw packaged_task_uninitialized();
        }
        task_->run( std::forward< Args >( args) ... );
    }

    void reset() {
        if ( ! valid() ) {
            throw packaged_task_uninitialized();
        }
        obtained_ = false;
        task_->reset();
    }
};

template< typename Signature >
void swap( packaged_task< Signature > & l, packaged_task< Signature > & r) {
    l.swap( r);
}

}}

#endif // BOOST_FIBERS_PACKAGED_TASK_HPP
