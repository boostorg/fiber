
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_FUTURE_HPP
#define BOOST_FIBERS_FUTURE_HPP

#include <boost/chrono/system_clocks.hpp>
#include <boost/config.hpp>
#include <boost/exception_ptr.hpp>
#include <boost/move/move.hpp>
#include <boost/system/system_error.hpp>
#include <boost/throw_exception.hpp>
#include <boost/utility/explicit_operator_bool.hpp>
#include <boost/utility.hpp>

#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/exceptions.hpp>
#include <boost/fiber/future/detail/shared_state.hpp>
#include <boost/fiber/future/future_status.hpp>

namespace boost {
namespace fibers {

template< typename S >
class packaged_task;

template< typename R >
class promise;

template< typename R >
class shared_future;

template< typename R >
class future : private noncopyable
{
private:
    typedef typename detail::shared_state< R >::ptr_t   ptr_t;

    friend class packaged_task< R() >;
    friend class promise< R >;
    friend class shared_future< R >;

    ptr_t           state_;

    BOOST_MOVABLE_BUT_NOT_COPYABLE( future);

    future( ptr_t const& p) :
        state_( p)
    {}

public:
    future() BOOST_NOEXCEPT :
        state_()
    {
        //TODO: constructs a future with no shared state
        //      after construction, valid() == false
    }

    ~future()
    {
        //TODO: abandon ownership if any
    }

    future( BOOST_RV_REF( future< R >) other) BOOST_NOEXCEPT :
        state_()
    {
        //TODO: constructs a future with the shared state of other using move semantics
        //      after construction, other.valid() == false
        swap( other);
    }

    future & operator=( BOOST_RV_REF( future< R >) other) BOOST_NOEXCEPT
    {
        //TODO: releases any shared state and move-assigns the contents of other to *this
        //      after the assignment, other.valid() == false and this->valid() will yield
        //      the same value as other.valid() before the assignment
        future tmp( boost::move( other) );
        swap( tmp);
        return * this;
    }

    void swap( future & other) BOOST_NOEXCEPT
    {
        //TODO: exchange the shared states of two futures
        state_.swap( other.state_);
    }

    BOOST_EXPLICIT_OPERATOR_BOOL();

    bool operator!() const BOOST_NOEXCEPT
    { return ! valid(); }

    bool valid() const BOOST_NOEXCEPT
    {
        //TODO: checks if the future refers to a shared state
        //      this is the case only for futures returned by
        //      promise::get_future(), packaged_task::get_future()
        //      or async() until the first time get()or share() is called
        return 0 != state_.get();
    }

    shared_future< R > share();

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
        tmp.swap( state_);
        return tmp->get();
    }

    exception_ptr get_exception_ptr()
    {
        if ( ! valid() )
            boost::throw_exception(
                future_uninitialized() );
        return state_->get_exception_ptr();
    }

    void wait() const
    {
        //TODO: blocks until the result becomes available
        //      valid() == true after the call
        if ( ! valid() )
            boost::throw_exception(
                future_uninitialized() );
        state_->wait();
    }

    template< class Rep, class Period >
    future_status wait_for( chrono::duration< Rep, Period > const& timeout_duration) const
    {
        //TODO: blocks until the result becomes available or timeout
        //      valid() == true after the call
        if ( ! valid() )
            boost::throw_exception(
                future_uninitialized() );
        return state_->wait_for( timeout_duration);
    }

    future_status wait_until( chrono::high_resolution_clock::time_point const& timeout_time) const
    {
        //TODO: blocks until the result becomes available or timeout
        //      valid() == true after the call
        if ( ! valid() )
            boost::throw_exception(
                future_uninitialized() );
        return state_->wait_until( timeout_time);
    }

    template< typename ClockType >
    future_status wait_until( typename ClockType::time_point const& timeout_time_) const
    {
        chrono::high_resolution_clock::time_point timeout_time(
            chrono::high_resolution_clock::now() + ( timeout_time_ - ClockType::now() ) );
        return wait_until( timeout_time);
    }
};

template< typename R >
class future< R & > : private noncopyable
{
private:
    typedef typename detail::shared_state< R & >::ptr_t   ptr_t;

    friend class packaged_task< R&() >;
    friend class promise< R & >;
    friend class shared_future< R & >;

    ptr_t           state_;

    BOOST_MOVABLE_BUT_NOT_COPYABLE( future);

    future( ptr_t const& p) :
        state_( p)
    {}

public:
    future() BOOST_NOEXCEPT :
        state_()
    {
        //TODO: constructs a future with no shared state
        //      after construction, valid() == false
    }

    ~future()
    {
        //TODO: abandon ownership if any
    }

    future( BOOST_RV_REF( future< R & >) other) BOOST_NOEXCEPT :
        state_()
    {
        //TODO: constructs a future with the shared state of other using move semantics
        //      after construction, other.valid() == false
        swap( other);
    }

    future & operator=( BOOST_RV_REF( future< R & >) other) BOOST_NOEXCEPT
    {
        //TODO: releases any shared state and move-assigns the contents of other to *this
        //      after the assignment, other.valid() == false and this->valid() will yield
        //      the same value as other.valid() before the assignment
        future tmp( boost::move( other) );
        swap( tmp);
        return * this;
    }

    void swap( future & other) BOOST_NOEXCEPT
    {
        //TODO: exchange the shared states of two futures
        state_.swap( other.state_);
    }

    BOOST_EXPLICIT_OPERATOR_BOOL();

    bool operator!() const BOOST_NOEXCEPT
    { return ! valid(); }

    bool valid() const BOOST_NOEXCEPT
    {
        //TODO: checks if the future refers to a shared state
        //      this is the case only for futures returned by
        //      promise::get_future(), packaged_task::get_future()
        //      or async() until the first time get()or share() is called
        return 0 != state_.get();
    }

    shared_future< R & > share();

    R & get()
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
        tmp.swap( state_);
        return tmp->get();
    }

    exception_ptr get_exception_ptr()
    {
        if ( ! valid() )
            boost::throw_exception(
                future_uninitialized() );
        return state_->get_exception_ptr();
    }

    void wait() const
    {
        //TODO: blocks until the result becomes available
        //      valid() == true after the call
        if ( ! valid() )
            boost::throw_exception(
                future_uninitialized() );
        state_->wait();
    }

    template< class Rep, class Period >
    future_status wait_for( chrono::duration< Rep, Period > const& timeout_duration) const
    {
        //TODO: blocks until the result becomes available or timeout
        //      valid() == true after the call
        if ( ! valid() )
            boost::throw_exception(
                future_uninitialized() );
        return state_->wait_for( timeout_duration);
    }

    future_status wait_until( chrono::high_resolution_clock::time_point const& timeout_time) const
    {
        //TODO: blocks until the result becomes available or timeout
        //      valid() == true after the call
        if ( ! valid() )
            boost::throw_exception(
                future_uninitialized() );
        return state_->wait_until( timeout_time);
    }

    template< typename ClockType >
    future_status wait_until( typename ClockType::time_point const& timeout_time_) const
    {
        chrono::high_resolution_clock::time_point timeout_time(
            chrono::high_resolution_clock::now() + ( timeout_time_ - ClockType::now() ) );
        return wait_until( timeout_time);
    }
};

template<>
class future< void > : private noncopyable
{
private:
    typedef detail::shared_state< void >::ptr_t   ptr_t;

    friend class packaged_task< void() >;
    friend class promise< void >;
    friend class shared_future< void >;

    ptr_t           state_;

    BOOST_MOVABLE_BUT_NOT_COPYABLE( future);

    future( ptr_t const& p) :
        state_( p)
    {}

public:
    future() BOOST_NOEXCEPT :
        state_()
    {
        //TODO: constructs a future with no shared state
        //      after construction, valid() == false
    }

    ~future()
    {
        //TODO: abandon ownership if any
    }

    inline future( BOOST_RV_REF( future< void >) other) BOOST_NOEXCEPT :
        state_()
    {
        //TODO: constructs a future with the shared state of other using move semantics
        //      after construction, other.valid() == false
        swap( other);
    }

    inline future & operator=( BOOST_RV_REF( future< void >) other) BOOST_NOEXCEPT
    {
        //TODO: releases any shared state and move-assigns the contents of other to *this
        //      after the assignment, other.valid() == false and this->valid() will yield
        //      the same value as other.valid() before the assignment
        future tmp( boost::move( other) );
        swap( tmp);
        return * this;
    }

    inline void swap( future & other) BOOST_NOEXCEPT
    {
        //TODO: exchange the shared states of two futures
        state_.swap( other.state_);
    }

    BOOST_EXPLICIT_OPERATOR_BOOL();

    inline bool operator!() const BOOST_NOEXCEPT
    { return ! valid(); }

    inline bool valid() const BOOST_NOEXCEPT
    {
        //TODO: checks if the future refers to a shared state
        //      this is the case only for futures returned by
        //      promise::get_future(), packaged_task::get_future()
        //      or async() until the first time get()or share() is called
        return 0 != state_.get();
    }

    shared_future< void > share();

    inline void get()
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
        tmp.swap( state_);
        tmp->get();
    }

    inline exception_ptr get_exception_ptr()
    {
        if ( ! valid() )
            boost::throw_exception(
                future_uninitialized() );
        return state_->get_exception_ptr();
    }

    inline void wait() const
    {
        //TODO: blocks until the result becomes available
        //      valid() == true after the call
        if ( ! valid() )
            boost::throw_exception(
                future_uninitialized() );
        state_->wait();
    }

    template< class Rep, class Period >
    future_status wait_for( chrono::duration< Rep, Period > const& timeout_duration) const
    {
        //TODO: blocks until the result becomes available or timeout
        //      valid() == true after the call
        if ( ! valid() )
            boost::throw_exception(
                future_uninitialized() );
        return state_->wait_for( timeout_duration);
    }

    inline future_status wait_until( chrono::high_resolution_clock::time_point const& timeout_time) const
    {
        //TODO: blocks until the result becomes available or timeout
        //      valid() == true after the call
        if ( ! valid() )
            boost::throw_exception(
                future_uninitialized() );
        return state_->wait_until( timeout_time);
    }

    template< typename ClockType >
    future_status wait_until( typename ClockType::time_point const& timeout_time_) const
    {
        chrono::high_resolution_clock::time_point timeout_time(
            chrono::high_resolution_clock::now() + ( timeout_time_ - ClockType::now() ) );
        return wait_until( timeout_time);
    }
};

template< typename R >
void swap( future< R > & l, future< R > & r)
{ l.swap( r); }



template< typename R >
class shared_future
{
private:
    typedef typename detail::shared_state< R >::ptr_t   ptr_t;

    friend class future< R >;

    ptr_t           state_;

    explicit shared_future( ptr_t const& p) :
        state_( p)
    {}

public:
    shared_future() BOOST_NOEXCEPT :
        state_()
    {
        //TODO: constructs a shared_future with no shared state
        //      after construction, valid() == false
    }

    ~shared_future()
    {
        //TODO: if *this is the last object referring to the shared state,
        //      destroys the shared state otherwise does nothing
    }

    shared_future( shared_future const& other) :
        state_( other.state_)
    {
        //TODO: constructs a shared future that refers to the same shared state,
        //      as other, if there's any
    }

    shared_future( BOOST_RV_REF( future< R >) other) BOOST_NOEXCEPT :
        state_()
    {
        //TODO: constructs a shared_future with the shared state of other using move semantics
        //      after construction, other.valid() == false
        state_.swap( other.state_);
    }

    shared_future( BOOST_RV_REF( shared_future) other) BOOST_NOEXCEPT :
        state_()
    {
        //TODO: constructs a shared_future with the shared state of other using move semantics
        //      after construction, other.valid() == false
        swap( other);
    }

    shared_future & operator=( BOOST_RV_REF( shared_future) other) BOOST_NOEXCEPT
    {
        //TODO: releases any shared state and move-assigns the contents of other to *this
        //      after the assignment, other.valid() == false and this->valid() will yield
        //      the same value as other.valid() before the assignment
        shared_future tmp( boost::move( other) );
        swap( tmp);
        return * this;
    }

    shared_future & operator=( shared_future const& other) BOOST_NOEXCEPT
    {
        //TODO:
        shared_future tmp( other);
        swap( tmp);
        return * this;
    }

    shared_future & operator=( BOOST_RV_REF( future< R >) other) BOOST_NOEXCEPT
    {
        //TODO:
        shared_future tmp( other);
        swap( tmp);
        return * this;
    }

    void swap( shared_future & other) BOOST_NOEXCEPT
    {
        //TODO: exchange the shared states of two shared_futures
        state_.swap( other.state_);
    }

    BOOST_EXPLICIT_OPERATOR_BOOL();

    bool operator!() const BOOST_NOEXCEPT
    { return ! valid(); }

    bool valid() const BOOST_NOEXCEPT
    {
        //TODO: checks if the shared_future refers to a shared state
        //      this is the case only for shared_futures returned by
        //      promise::get_shared_future(), packaged_task::get_shared_future()
        //      or async() until the first time get()or share() is called
        return 0 != state_.get();
    }

    R const& get() const
    {
        //TODO: the get method waits until the shared_future has a valid result and
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
        return state_->get();
    }

    exception_ptr get_exception_ptr()
    {
        if ( ! valid() )
            boost::throw_exception(
                future_uninitialized() );
        return state_->get_exception_ptr();
    }

    void wait() const
    {
        //TODO: blocks until the result becomes available
        //      valid() == true after the call
        if ( ! valid() )
            boost::throw_exception(
                future_uninitialized() );
        state_->wait();
    }

    template< class Rep, class Period >
    future_status wait_for( chrono::duration< Rep, Period > const& timeout_duration) const
    {
        //TODO: blocks until the result becomes available or timeout
        //      valid() == true after the call
        if ( ! valid() )
            boost::throw_exception(
                future_uninitialized() );
        return state_->wait_for( timeout_duration);
    }

    future_status wait_until( chrono::high_resolution_clock::time_point const& timeout_time) const
    {
        //TODO: blocks until the result becomes available or timeout
        //      valid() == true after the call
        if ( ! valid() )
            boost::throw_exception(
                future_uninitialized() );
        return state_->wait_until( timeout_time);
    }

    template< typename ClockType >
    future_status wait_until( typename ClockType::time_point const& timeout_time_) const
    {
        chrono::high_resolution_clock::time_point timeout_time(
            chrono::high_resolution_clock::now() + ( timeout_time_ - ClockType::now() ) );
        return wait_until( timeout_time);
    }
};

template< typename R >
class shared_future< R & >
{
private:
    typedef typename detail::shared_state< R & >::ptr_t   ptr_t;

    friend class future< R & >;

    ptr_t           state_;

    explicit shared_future( ptr_t const& p) :
        state_( p)
    {}

public:
    shared_future() BOOST_NOEXCEPT :
        state_()
    {
        //TODO: constructs a shared_future with no shared state
        //      after construction, valid() == false
    }

    ~shared_future()
    {
        //TODO: if *this is the last object referring to the shared state,
        //      destroys the shared state otherwise does nothing
    }

    shared_future( shared_future const& other) :
        state_( other.state_)
    {
        //TODO: constructs a shared future that refers to the same shared state,
        //      as other, if there's any
    }

    shared_future( BOOST_RV_REF( future< R & >) other) BOOST_NOEXCEPT :
        state_()
    {
        //TODO: constructs a shared_future with the shared state of other using move semantics
        //      after construction, other.valid() == false
        state_.swap( other.state_);
    }

    shared_future( BOOST_RV_REF( shared_future) other) BOOST_NOEXCEPT :
        state_()
    {
        //TODO: constructs a shared_future with the shared state of other using move semantics
        //      after construction, other.valid() == false
        swap( other);
    }

    shared_future & operator=( BOOST_RV_REF( shared_future) other) BOOST_NOEXCEPT
    {
        //TODO: releases any shared state and move-assigns the contents of other to *this
        //      after the assignment, other.valid() == false and this->valid() will yield
        //      the same value as other.valid() before the assignment
        shared_future tmp( boost::move( other) );
        swap( tmp);
        return * this;
    }

    shared_future & operator=( shared_future const& other) BOOST_NOEXCEPT
    {
        //TODO:
        shared_future tmp( other);
        swap( tmp);
        return * this;
    }

    shared_future & operator=( BOOST_RV_REF( future< R & >) other) BOOST_NOEXCEPT
    {
        //TODO:
        shared_future tmp( other);
        swap( tmp);
        return * this;
    }

    void swap( shared_future & other) BOOST_NOEXCEPT
    {
        //TODO: exchange the shared states of two shared_futures
        state_.swap( other.state_);
    }

    BOOST_EXPLICIT_OPERATOR_BOOL();

    bool operator!() const BOOST_NOEXCEPT
    { return ! valid(); }

    bool valid() const BOOST_NOEXCEPT
    {
        //TODO: checks if the shared_future refers to a shared state
        //      this is the case only for shared_futures returned by
        //      promise::get_shared_future(), packaged_task::get_shared_future()
        //      or async() until the first time get()or share() is called
        return 0 != state_.get();
    }

    R & get() const
    {
        //TODO: the get method waits until the shared_future has a valid result and
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
        return state_->get();
    }

    exception_ptr get_exception_ptr()
    {
        if ( ! valid() )
            boost::throw_exception(
                future_uninitialized() );
        return state_->get_exception_ptr();
    }

    void wait() const
    {
        //TODO: blocks until the result becomes available
        //      valid() == true after the call
        if ( ! valid() )
            boost::throw_exception(
                future_uninitialized() );
        state_->wait();
    }

    template< class Rep, class Period >
    future_status wait_for( chrono::duration< Rep, Period > const& timeout_duration) const
    {
        //TODO: blocks until the result becomes available or timeout
        //      valid() == true after the call
        if ( ! valid() )
            boost::throw_exception(
                future_uninitialized() );
        return state_->wait_for( timeout_duration);
    }

    future_status wait_until( chrono::high_resolution_clock::time_point const& timeout_time) const
    {
        //TODO: blocks until the result becomes available or timeout
        //      valid() == true after the call
        if ( ! valid() )
            boost::throw_exception(
                future_uninitialized() );
        return state_->wait_until( timeout_time);
    }

    template< typename ClockType >
    future_status wait_until( typename ClockType::time_point const& timeout_time_) const
    {
        chrono::high_resolution_clock::time_point timeout_time(
            chrono::high_resolution_clock::now() + ( timeout_time_ - ClockType::now() ) );
        return wait_until( timeout_time);
    }
};

template<>
class shared_future< void >
{
private:
    typedef detail::shared_state< void >::ptr_t   ptr_t;

    friend class future< void >;

    ptr_t           state_;

    shared_future( ptr_t const& p) :
        state_( p)
    {}

public:
    shared_future() BOOST_NOEXCEPT :
        state_()
    {
        //TODO: constructs a shared_future with no shared state
        //      after construction, valid() == false
    }

    ~shared_future()
    {
        //TODO: if *this is the last object referring to the shared state,
        //      destroys the shared state otherwise does nothing
    }

    inline shared_future( shared_future const& other) :
        state_( other.state_)
    {
        //TODO: constructs a shared future that refers to the same shared state,
        //      as other, if there's any
    }

    inline shared_future( BOOST_RV_REF( future< void >) other) BOOST_NOEXCEPT :
        state_()
    {
        //TODO: constructs a shared_future with the shared state of other using move semantics
        //      after construction, other.valid() == false
        state_.swap( other.state_);
    }

    inline shared_future( BOOST_RV_REF( shared_future) other) BOOST_NOEXCEPT :
        state_()
    {
        //TODO: constructs a shared_future with the shared state of other using move semantics
        //      after construction, other.valid() == false
        swap( other);
    }

    inline shared_future & operator=( BOOST_RV_REF( shared_future) other) BOOST_NOEXCEPT
    {
        //TODO: releases any shared state and move-assigns the contents of other to *this
        //      after the assignment, other.valid() == false and this->valid() will yield
        //      the same value as other.valid() before the assignment
        shared_future tmp( boost::move( other) );
        swap( tmp);
        return * this;
    }

    inline shared_future & operator=( shared_future const& other) BOOST_NOEXCEPT
    {
        //TODO:
        shared_future tmp( boost::move( other) );
        swap( tmp);
        return * this;
    }

    inline shared_future & operator=( BOOST_RV_REF( future< void >) other) BOOST_NOEXCEPT
    {
        //TODO:
        shared_future tmp( boost::move( other) );
        swap( tmp);
        return * this;
    }

    inline void swap( future< void > & other) BOOST_NOEXCEPT
    {
        //TODO: exchange the shared states of two shared_futures
        state_.swap( other.state_);
    }

    inline void swap( shared_future & other) BOOST_NOEXCEPT
    {
        //TODO: exchange the shared states of two shared_futures
        state_.swap( other.state_);
    }

    BOOST_EXPLICIT_OPERATOR_BOOL();

    inline bool operator!() const BOOST_NOEXCEPT
    { return ! valid(); }

    inline bool valid() const BOOST_NOEXCEPT
    {
        //TODO: checks if the shared_future refers to a shared state
        //      this is the case only for shared_futures returned by
        //      promise::get_shared_future(), packaged_task::get_shared_future()
        //      or async() until the first time get()or share() is called
        return 0 != state_.get();
    }

    inline void get() const
    {
        //TODO: the get method waits until the shared_future has a valid result and
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
        state_->get();
    }

    inline exception_ptr get_exception_ptr()
    {
        if ( ! valid() )
            boost::throw_exception(
                future_uninitialized() );
        return state_->get_exception_ptr();
    }

    inline void wait() const
    {
        //TODO: blocks until the result becomes available
        //      valid() == true after the call
        if ( ! valid() )
            boost::throw_exception(
                future_uninitialized() );
        state_->wait();
    }

    template< class Rep, class Period >
    future_status wait_for( chrono::duration< Rep, Period > const& timeout_duration) const
    {
        //TODO: blocks until the result becomes available or timeout
        //      valid() == true after the call
        if ( ! valid() )
            boost::throw_exception(
                future_uninitialized() );
        return state_->wait_for( timeout_duration);
    }

    inline future_status wait_until( chrono::high_resolution_clock::time_point const& timeout_time) const
    {
        //TODO: blocks until the result becomes available or timeout
        //      valid() == true after the call
        if ( ! valid() )
            boost::throw_exception(
                future_uninitialized() );
        return state_->wait_until( timeout_time);
    }

    template< typename ClockType >
    future_status wait_until( typename ClockType::time_point const& timeout_time_) const
    {
        chrono::high_resolution_clock::time_point timeout_time(
            chrono::high_resolution_clock::now() + ( timeout_time_ - ClockType::now() ) );
        return wait_until( timeout_time);
    }
};

template< typename R >
void swap( shared_future< R > & l, shared_future< R > & r)
{ l.swap( r); }



template< typename R >
shared_future< R >
future< R >::share()
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
    return shared_future< R >( boost::move( * this) );
}

template< typename R >
shared_future< R & >
future< R & >::share()
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
    return shared_future< R & >( boost::move( * this) );
}

inline
shared_future< void >
future< void >::share()
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
    return shared_future< void >( boost::move( * this) );
}

}}

#endif
