
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_PACKAGED_TASK_HPP
#define BOOST_FIBERS_PACKAGED_TASK_HPP

#include <boost/config.hpp>
#include <boost/move/move.hpp>
#include <boost/throw_exception.hpp>
#include <boost/utility.hpp>

#include <boost/fiber/exceptions.hpp>
#include <boost/fiber/future/future.hpp>

namespace boost {
namespace fibers {

template< typename Signature >
class packaged_task : private noncopyable
{
private:
    struct dummy
    { void nonnull() {} };

    typedef void ( dummy::*safe_bool)();

    atomic< bool >  obtained_;

    BOOST_MOVABLE_BUT_NOT_COPYABLE( packaged_task);

public:
    packaged_task()
    {
        //TODO: constructs a packaged_task object with
        //       no task and no shared state
    }

#ifndef BOOST_NO_RVALUE_REFERENCES
    template< typename Fn >
    explicit packaged_task( Fn && fn)
    {
        //TODO: constructs a std::packaged_task object
        //       with a shared state and a copy of the task,
        //       initialized with forward< Fn >( fn)
    }

    template< typename Fn, typename Allocator >
    explicit packaged_task( Allocator const& alloc, Fn && fn)
    {
        //TODO: constructs a std::packaged_task object
        //       with a shared state and a copy of the task,
        //       initialized with forward< Fn >( fn)
        //       uses the provided allocator to allocate
        //       memory necessary to store the task
    }

    packaged_task( packaged_task && other) BOOST_NOEXCEPT
    {
        //TODO: constructs a std::packaged_task with thes
        //       shared state and task formerly owned by rhs,
        //       leaving rhs with no shared state and a moved-from task
        swap( other);
    }

    packaged_task & operator=( packaged_task && other) BOOST_NOEXCEPT
    {
        //TODO: releases the shared state, if any, destroys the
        //       previously-held task, and moves the shared state
        //       and the task owned by rhs into *this
        //       rhs is left without a shared state and with a
        //       moved-from task
        packaged_task tmp( boost::move( other) );
        swap( tmp);
        return * this;
    }
#else
    template< typename Fn >
    explicit packaged_task( BOOST_RV_REF( Fn) fn)
    {
        //TODO: constructs a std::packaged_task object
        //       with a shared state and a copy of the task,
        //       initialized with forward< Fn >( fn)
    }

    template< typename Fn, typename Allocator >
    explicit packaged_task( Allocator const& alloc, BOOST_RV_REF( Fn) fn)
    {
        //TODO: constructs a std::packaged_task object
        //       with a shared state and a copy of the task,
        //       initialized with forward< Fn >( fn)
        //       uses the provided allocator to allocate
        //       memory necessary to store the task
    }

    packaged_task( BOOST_RV_REF( packaged_task) other) BOOST_NOEXCEPT
    {
        //TODO: constructs a std::packaged_task with thes
        //       shared state and task formerly owned by rhs,
        //       leaving rhs with no shared state and a moved-from task
        swap( other);
    }

    packaged_task & operator=( BOOST_RV_REF( packaged_task) other) BOOST_NOEXCEPT
    {
        //TODO: releases the shared state, if any, destroys the
        //       previously-held task, and moves the shared state
        //       and the task owned by rhs into *this
        //       rhs is left without a shared state and with a
        //       moved-from task
        packaged_task tmp( boost::move( other) );
        swap( tmp);
        return * this;
    }
#endif

    void swap( promise & other) BOOST_NOEXCEPT
    {
        //TODO: exchange the shared states of two packaged_task
    }

    operator safe_bool() const BOOST_NOEXCEPT
    { return valid() ? & dummy::nonnull : 0; }

    bool operator!() const BOOST_NOEXCEPT
    { return ! valid(); }

    bool valid() const BOOST_NOEXCEPT
    {
        //TODO: checks whether *this has a shared state
        return future_;
    }

    future< R > get_future()
    {
        //TODO: eturns a future which shares the same shared state as *this
        //      get_future can be called only once for each packaged_task
        if ( ! prom_)
            boost::throw_exception(
                packaged_task_uninitialized() );
        return prom_->get_future();
    }

    void operator()(...)
    {
        //TODO: calls the stored task with args as the arguments
        //      the return value of the task or any exceptions thrown are
        //      stored in the shared state
        //      the shared state is made ready and any threads waiting for
        //      this are unblocked
    }

    void reset()
    {
        //TODO: resets the state abandoning the results of previous executions
        //      new shared state is constructed
        //      equivalent to *this = packaged_task(std::move(f)), where f is
        //      the stored task
    }
};

template< typename Signature >
void swap( packaged_task< Signature > & l, packaged_task< Signature > & r)
{ l.swap( r); }

}}

#endif // BOOST_FIBERS_PACKAGED_TASK_HPP
