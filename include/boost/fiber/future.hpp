
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_FUTURE_HPP
#define BOOST_FIBERS_FUTURE_HPP

#include <boost/config.hpp>
#include <boost/move/move.hpp>
#include <boost/system/system_error.hpp>
#include <boost/throw_exception.hpp>
#include <boost/utility.hpp>

#include <boost/fiber/detail/future_base.hpp>
#include <boost/fiber/exceptions.hpp>

namespace boost {
namespace fibers {

template< typename R >
class promise;

template< typename R >
class future : private noncopyable
{
private:
    typedef typename detail::future_base< R >::ptr_t   ptr_t;

    friend class promise< R >;

    struct dummy
    { void nonnull() {} };

    typedef void ( dummy::*safe_bool)();

    ptr_t           future_;

    BOOST_MOVABLE_BUT_NOT_COPYABLE( future);

    future( ptr_t const& p) :
        future_( p)
    {}

public:
    future() BOOST_NOEXCEPT :
        future_()
    {
        //TODO: constructs a future with no shared state
        //      after construction, valid() == false
    }

    ~future()
    {
        //TODO: abandon ownership if any
    }

#ifndef BOOST_NO_RVALUE_REFERENCES
    future( future && other) BOOST_NOEXCEPT :
        future_()
    {
        //TODO: constructs a future with the shared state of other using move semantics
        //      after construction, other.valid() == false
        swap( other);
    }

    future & operator=( future && other) BOOST_NOEXCEPT
    {
        //TODO: releases any shared state and move-assigns the contents of other to *this
        //      after the assignment, other.valid() == false and this->valid() will yield
        //      the same value as other.valid() before the assignment
        future tmp( boost::move( other) );
        swap( tmp);
        return * this;
    }
#else
    future( BOOST_RV_REF( future) other) BOOST_NOEXCEPT :
        future_()
    {
        //TODO: constructs a future with the shared state of other using move semantics
        //      after construction, other.valid() == false
        swap( other);
    }

    future & operator=( BOOST_RV_REF( future) other) BOOST_NOEXCEPT
    {
        //TODO: releases any shared state and move-assigns the contents of other to *this
        //      after the assignment, other.valid() == false and this->valid() will yield
        //      the same value as other.valid() before the assignment
        future tmp( boost::move( other) );
        swap( tmp);
        return * this;
    }
#endif   

    void swap( future & other) BOOST_NOEXCEPT
    {
        //TODO: exchange the shared states of two futures
        future_.swap( other.future_);
    }

    operator safe_bool() const BOOST_NOEXCEPT
    { return valid() ? & dummy::nonnull : 0; }

    bool operator!() const BOOST_NOEXCEPT
    { return ! valid(); }

    bool valid() const BOOST_NOEXCEPT
    {
        //TODO: checks if the future refers to a shared state
        //      this is the case only for futures returned by
        //      promise::get_future(), packaged_task::get_future()
        //      or async() until the first time get()or share() is called
        return future_;
    }
#if 0
    shared_future< R > share()
    {
        //TODO: transfer the shared state of *this to a shared_future object
        //      multiple shared_future objects may reference the same shared state,
        //      which is not possible with future
        //      after calling share on a future, valid() == false
        //      detect the case when valid == false before the call and throw a
        //      future_error with an error condition of future_errc::no_state
        if ( ! valid() )
            boost::throw_exception(
                future_uninitialized() );
        ptr_t tmp;
        tmp.swap( future_);
        return shared_future< R >( tmp);
    }
#endif
    R get()
    {
        //TODO: the get method waits until the future has a valid result and
        //      (depending on which template is used) retrieves it
        //      it effectively calls wait() in order to wait for the result
        //      the value stored in the shared state
        //      if it satisfies the requirements of MoveAssignable, the value is moved,
        //      otherwise it is copied
        //      valid() == false after a call to this method.
        //      detect the case when valid == false before the call and throw a
        //      future_error with an error condition of future_errc::no_state
        if ( ! valid() )
            boost::throw_exception(
                future_uninitialized() );
        ptr_t tmp;
        tmp.swap( future_);
        return tmp->get();
    }

    void wait() const
    {
        //TODO: blocks until the result becomes available
        //      valid() == true after the call
        if ( ! valid() )
            boost::throw_exception(
                future_uninitialized() );
        future_->wait();
    }
};

template<>
class future< void > : private noncopyable
{
private:
    typedef typename detail::future_base< void >::ptr_t   ptr_t;

    friend class promise< void >;

    struct dummy
    { void nonnull() {} };

    typedef void ( dummy::*safe_bool)();

    ptr_t           future_;

    BOOST_MOVABLE_BUT_NOT_COPYABLE( future);

    future( ptr_t const& p) :
        future_( p)
    {}

public:
    future() BOOST_NOEXCEPT :
        future_()
    {
        //TODO: constructs a future with no shared state
        //      after construction, valid() == false
    }

    ~future()
    {
        //TODO: abandon ownership if any
    }

#ifndef BOOST_NO_RVALUE_REFERENCES
    future( future && other) BOOST_NOEXCEPT :
        future_()
    {
        //TODO: constructs a future with the shared state of other using move semantics
        //      after construction, other.valid() == false
        swap( other);
    }

    future & operator=( future && other) BOOST_NOEXCEPT
    {
        //TODO: releases any shared state and move-assigns the contents of other to *this
        //      after the assignment, other.valid() == false and this->valid() will yield
        //      the same value as other.valid() before the assignment
        future tmp( boost::move( other) );
        swap( tmp);
        return * this;
    }
#else
    future( BOOST_RV_REF( future) other) BOOST_NOEXCEPT :
        future_()
    {
        //TODO: constructs a future with the shared state of other using move semantics
        //      after construction, other.valid() == false
        swap( other);
    }

    future & operator=( BOOST_RV_REF( future) other) BOOST_NOEXCEPT
    {
        //TODO: releases any shared state and move-assigns the contents of other to *this
        //      after the assignment, other.valid() == false and this->valid() will yield
        //      the same value as other.valid() before the assignment
        future tmp( boost::move( other) );
        swap( tmp);
        return * this;
    }
#endif

    void swap( future & other) BOOST_NOEXCEPT
    {
        //TODO: exchange the shared states of two futures
        future_.swap( other.future_);
    }

    operator safe_bool() const BOOST_NOEXCEPT
    { return valid() ? & dummy::nonnull : 0; }

    bool operator!() const BOOST_NOEXCEPT
    { return ! valid(); }

    bool valid() const BOOST_NOEXCEPT
    {
        //TODO: checks if the future refers to a shared state
        //      this is the case only for futures returned by
        //      promise::get_future(), packaged_task::get_future()
        //      or async() until the first time get()or share() is called
        return future_;
    }
#if 0
    shared_future< void > share()
    {
        //TODO: transfer the shared state of *this to a shared_future object
        //      multiple shared_future objects may reference the same shared state,
        //      which is not possible with future
        //      after calling share on a future, valid() == false
        //      detect the case when valid == false before the call and throw a
        //      future_error with an error condition of future_errc::no_state
        if ( ! valid() )
            boost::throw_exception(
                future_uninitialized() );
        ptr_t tmp;
        tmp.swap( future_);
        return shared_future< void >( tmp);
    }
#endif
    void get()
    {
        //TODO: the get method waits until the future has a valid result and
        //      (depending on which template is used) retrieves it
        //      it effectively calls wait() in order to wait for the result
        //      the value stored in the shared state
        //      if it satisfies the requirements of MoveAssignable, the value is moved,
        //      otherwise it is copied
        //      valid() == false after a call to this method.
        //      detect the case when valid == false before the call and throw a
        //      future_error with an error condition of future_errc::no_state
        if ( ! valid() )
            boost::throw_exception(
                future_uninitialized() );
        ptr_t tmp;
        tmp.swap( future_);
        tmp->get();
    }

    void wait() const
    {
        //TODO: blocks until the result becomes available
        //      valid() == true after the call
        if ( ! valid() )
            boost::throw_exception(
                future_uninitialized() );
        future_->wait();
    }
};

template< typename R >
void swap( future< R > & l, future< R > & r)
{ l.swap( r); }

}}

#endif
