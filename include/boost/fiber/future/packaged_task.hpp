
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

template< typename R >
class packaged_task< R() > {
private:
    typedef typename detail::task_base< R >::ptr_t   ptr_t;

    bool            obtained_;
    ptr_t           task_;

public:
    packaged_task() noexcept :
        obtained_( false),
        task_() {
        //TODO: constructs a packaged_task object with
        //       no task and no shared state
    }

    ~packaged_task() {
        //TODO: abandons the shared state and destroys the stored task object
        //      a usual, if the shared state is abandoned before it was made
        //      ready, an std::future_error exception is stored with the error
        //      code future_errc::broken_promise
        if ( task_) {
            task_->owner_destroyed();
        }
    }

    template< typename Fn >
    explicit packaged_task( Fn && fn) :
        obtained_( false),
        task_() {
        //TODO: constructs a std::packaged_task object
        //       with a shared state and a copy of the task,
        //       initialized with std::forward< Fn >( fn)
        typedef detail::task_object<
            Fn,
            std::allocator< packaged_task< R() > >,
            R
        >                                       object_t;
        std::allocator< packaged_task< R() > > alloc;
        typename object_t::allocator_t a( alloc);
        task_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( std::forward< Fn >( fn), a) );
    }

    template< typename Fn, typename Allocator >
    explicit packaged_task( std::allocator_arg_t, Allocator const& alloc, Fn && fn) :
        obtained_( false),
        task_() {
        //TODO: constructs a std::packaged_task object
        //       with a shared state and a copy of the task,
        //       initialized with std::forward< Fn >( fn)
        //       uses the provided allocator to allocate
        //       memory necessary to store the task
        typedef detail::task_object<
            Fn,
            Allocator,
            R
        >                                       object_t;
        typename object_t::allocator_t a( alloc);
        task_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( std::forward< Fn >( fn), a) );
    }

    packaged_task( packaged_task const&) = delete;
    packaged_task & operator=( packaged_task const&) = delete;

    packaged_task( packaged_task && other) noexcept :
        obtained_( other.obtained_),
        task_( std::move( other.task_) ) {
        //TODO: constructs a std::packaged_task with thes
        //       shared state and task formerly owned by rhs,
        //       leaving rhs with no shared state and a moved-from task
        other.obtained_ = false;
    }

    packaged_task & operator=( packaged_task && other) noexcept {
        //TODO: releases the shared state, if any, destroys the
        //       previously-held task, and moves the shared state
        //       and the task owned by rhs into *this
        //       rhs is left without a shared state and with a
        //       moved-from task
        if ( this != & other) {
            obtained_ = other.obtained_;
            other.obtained_ = false;
            task_ = std::move( other.task_);
        }
        return * this;
    }

    void swap( packaged_task & other) noexcept {
        //TODO: exchange the shared states of two packaged_task
        std::swap( obtained_, other.obtained_);
        task_.swap( other.task_);
    }

    bool valid() const noexcept {
        //TODO: checks whether *this has a shared state
        return nullptr != task_.get();
    }

    future< R > get_future() {
        //TODO: returns a future which shares the same shared state as *this
        //      get_future can be called only once for each packaged_task
        if ( obtained_) {
            throw future_already_retrieved();
        }
        if ( ! valid() ) {
            throw packaged_task_uninitialized();
        }
        obtained_ = true;
        return future< R >( task_);
    }

    void operator()() {
        //TODO: calls the stored task with args as the arguments
        //      the return value of the task or any exceptions thrown are
        //      stored in the shared state
        //      the shared state is made ready and any threads waiting for
        //      this are unblocked
        if ( ! valid() ) {
            throw packaged_task_uninitialized();
        }
        task_->run();
    }

    void reset() {
        //TODO: resets the state abandoning the results of previous executions
        //      new shared state is constructed
        //      equivalent to *this = packaged_task(std::move(f)), where f is
        //      the stored task
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
