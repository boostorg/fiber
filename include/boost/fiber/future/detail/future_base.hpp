
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_DETAIL_FUTURE_BASE_H
#define BOOST_FIBERS_DETAIL_FUTURE_BASE_H

#include <cstddef>

#include <boost/assert.hpp>
#include <boost/atomic.hpp>
#include <boost/config.hpp>
#include <boost/exception_ptr.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/move/move.hpp>
#include <boost/optional.hpp>
#include <boost/thread/lock_types.hpp>
#include <boost/utility.hpp>

#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/condition.hpp>
#include <boost/fiber/exceptions.hpp>
#include <boost/fiber/mutex.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace detail {

template< typename R >
class future_base : public noncopyable
{
private:
    atomic< std::size_t >   use_count_;
    mutable mutex           mtx_;
    mutable condition       waiters_;
    bool                    ready_;
    optional< R >           value_;
    exception_ptr           except_;

    void mark_ready_and_notify_()
    {
        ready_ = true;
        waiters_.notify_all();
    }

    void owner_destroyed_()
    {
        //TODO: set broken_exception if future was not already done
        //      notify all waiters
        if ( ! ready_)
            set_exception_( copy_exception( broken_promise() ) );
    }

    void set_value_( R const& value)
    {
        //TODO: store the value and make the future ready
        //      notify all waiters
        if ( ready_)
            boost::throw_exception(
                promise_already_satisfied() );
        value_ = value;
        mark_ready_and_notify_();
    }

#ifndef BOOST_NO_RVALUE_REFERENCES
    void set_value_( R && value)
    {
        //TODO: store the value and make the future ready
        //      notify all waiters
        if ( ready_)
            boost::throw_exception(
                promise_already_satisfied() );
        value_ = boost::move( value);
        mark_ready_and_notify_();
    }
#else
    void set_value_( BOOST_RV_REF( R) value)
    {
        //TODO: store the value and make the future ready
        //      notify all waiters
        if ( ready_)
            boost::throw_exception(
                promise_already_satisfied() );
        value_ = boost::move( value);
        mark_ready_and_notify_();
    }
#endif

    void set_exception_( exception_ptr except)
    {
        //TODO: store the exception pointer p into the shared state and make the state ready
        //      done = true, notify all waiters
        if ( ready_)
            boost::throw_exception(
                promise_already_satisfied() );
        except_ = except;
        mark_ready_and_notify_();
    }

    R get_( unique_lock< mutex > & lk)
    {
        //TODO: the get method waits until the future has a valid result and
        //      (depending on which template is used) retrieves it
        //      it effectively calls wait_() in order to wait for the result
        //      if it satisfies the requirements of MoveAssignable, the value is moved,
        //      otherwise it is copied
        wait_( lk);
        if ( except_)
            rethrow_exception( except_);
        return value_.get();
    }

    void wait_( unique_lock< mutex > & lk) const
    {
        //TODO: blocks until the result becomes available
        while ( ! ready_)
            waiters_.wait( lk);
    }

protected:
    virtual void deallocate_future() = 0;

public:
    typedef intrusive_ptr< future_base >    ptr_t;

    future_base() :
        use_count_( 0), mtx_(), ready_( false),
        value_(), except_()
    {}

    virtual ~future_base() {}

    void owner_destroyed()
    {
        //TODO: lock mutex
        //      set broken_exception if future was not already done
        //      done = true, notify all waiters
        unique_lock< mutex > lk( mtx_);
        owner_destroyed_();
    }

    void set_value( R const& value)
    {
        //TODO: store the value into the shared state and make the state ready
        //      the operation is atomic, i.e. it behaves as though they acquire a single mutex
        //      associated with the promise object while updating the promise object
        //      an exception is thrown if there is no shared state or the shared state already
        //      stores a value or exception
        unique_lock< mutex > lk( mtx_);
        set_value_( value);
    }

#ifndef BOOST_NO_RVALUE_REFERENCES
    void set_value( R && value)
    {
        //TODO: store the value into the shared state and make the state ready
        //      rhe operation is atomic, i.e. it behaves as though they acquire a single mutex
        //      associated with the promise object while updating the promise object
        //      an exception is thrown if there is no shared state or the shared state already
        //      stores a value or exception
        unique_lock< mutex > lk( mtx_);
        set_value_( boot::move( value) );
    }
#else
    void set_value( BOOST_RV_REF( R) value)
    {
        //TODO: store the value into the shared state and make the state ready
        //      rhe operation is atomic, i.e. it behaves as though they acquire a single mutex
        //      associated with the promise object while updating the promise object
        //      an exception is thrown if there is no shared state or the shared state already
        //      stores a value or exception
        unique_lock< mutex > lk( mtx_);
        set_value_( boost::move( value) );
    }
#endif

    void set_exception( exception_ptr except)
    {
        //TODO: store the exception pointer p into the shared state and make the state ready
        //      the operation is atomic, i.e. it behaves as though they acquire a single mutex
        //      associated with the promise object while updating the promise object
        //      an exception is thrown if there is no shared state or the shared state already
        //      stores a value or exception
        unique_lock< mutex > lk( mtx_);
        set_exception_( except);
    }

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
        unique_lock< mutex > lk( mtx_);
        return get_( lk);
    }

    void wait() const
    {
        //TODO: blocks until the result becomes available
        //      valid() == true after the call
        unique_lock< mutex > lk( mtx_);
        wait_( lk);
    }

    friend inline void intrusive_ptr_add_ref( future_base * p) BOOST_NOEXCEPT
    { ++p->use_count_; }

    friend inline void intrusive_ptr_release( future_base * p)
    {
        if ( 0 == --p->use_count_)
           p->deallocate_future();
    }
};

template< typename R >
class future_base< R & > : public noncopyable
{
private:
    atomic< std::size_t >   use_count_;
    mutable mutex           mtx_;
    mutable condition       waiters_;
    bool                    ready_;
    R                   *   value_;
    exception_ptr           except_;

    void mark_ready_and_notify_()
    {
        ready_ = true;
        waiters_.notify_all();
    }

    void owner_destroyed_()
    {
        //TODO: set broken_exception if future was not already done
        //      notify all waiters
        if ( ! ready_)
            set_exception_( copy_exception( broken_promise() ) );
    }

    void set_value_( R & value)
    {
        //TODO: store the value and make the future ready
        //      notify all waiters
        if ( ready_)
            boost::throw_exception(
                promise_already_satisfied() );
        value_ = & value;
        mark_ready_and_notify_();
    }

    void set_exception_( exception_ptr except)
    {
        //TODO: store the exception pointer p into the shared state and make the state ready
        //      done = true, notify all waiters
        if ( ready_)
            boost::throw_exception(
                promise_already_satisfied() );
        except_ = except;
        mark_ready_and_notify_();
    }

    R & get_( unique_lock< mutex > & lk)
    {
        //TODO: the get method waits until the future has a valid result and
        //      (depending on which template is used) retrieves it
        //      it effectively calls wait_() in order to wait for the result
        //      if it satisfies the requirements of MoveAssignable, the value is moved,
        //      otherwise it is copied
        wait_( lk);
        if ( except_)
            rethrow_exception( except_);
        return * value_;
    }

    void wait_( unique_lock< mutex > & lk) const
    {
        //TODO: blocks until the result becomes available
        while ( ! ready_)
            waiters_.wait( lk);
    }

protected:
    virtual void deallocate_future() = 0;

public:
    typedef intrusive_ptr< future_base >    ptr_t;

    future_base() :
        use_count_( 0), mtx_(), ready_( false),
        value_( 0), except_()
    {}

    virtual ~future_base() {}

    void owner_destroyed()
    {
        //TODO: lock mutex
        //      set broken_exception if future was not already done
        //      done = true, notify all waiters
        unique_lock< mutex > lk( mtx_);
        owner_destroyed_();
    }

    void set_value( R & value)
    {
        //TODO: store the value into the shared state and make the state ready
        //      the operation is atomic, i.e. it behaves as though they acquire a single mutex
        //      associated with the promise object while updating the promise object
        //      an exception is thrown if there is no shared state or the shared state already
        //      stores a value or exception
        unique_lock< mutex > lk( mtx_);
        set_value_( value);
    }

    void set_exception( exception_ptr except)
    {
        //TODO: store the exception pointer p into the shared state and make the state ready
        //      the operation is atomic, i.e. it behaves as though they acquire a single mutex
        //      associated with the promise object while updating the promise object
        //      an exception is thrown if there is no shared state or the shared state already
        //      stores a value or exception
        unique_lock< mutex > lk( mtx_);
        set_exception_( except);
    }

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
        unique_lock< mutex > lk( mtx_);
        return get_( lk);
    }

    void wait() const
    {
        //TODO: blocks until the result becomes available
        //      valid() == true after the call
        unique_lock< mutex > lk( mtx_);
        wait_( lk);
    }

    friend inline void intrusive_ptr_add_ref( future_base * p) BOOST_NOEXCEPT
    { ++p->use_count_; }

    friend inline void intrusive_ptr_release( future_base * p)
    {
        if ( 0 == --p->use_count_)
           p->deallocate_future();
    }
};

template<>
class future_base< void > : public noncopyable
{
private:
    atomic< std::size_t >   use_count_;
    mutable mutex           mtx_;
    mutable condition       waiters_;
    bool                    ready_;
    exception_ptr           except_;

    void mark_ready_and_notify_()
    {
        ready_ = true;
        waiters_.notify_all();
    }

    void owner_destroyed_()
    {
        //TODO: set broken_exception if future was not already done
        //      notify all waiters
        if ( ! ready_)
            set_exception_( copy_exception( broken_promise() ) );
    }

    void set_value_()
    {
        //TODO: store the value and make the future ready
        //      notify all waiters
        if ( ready_)
            boost::throw_exception(
                promise_already_satisfied() );
        mark_ready_and_notify_();
    }

    void set_exception_( exception_ptr except)
    {
        //TODO: store the exception pointer p into the shared state and make the state ready
        //      done = true, notify all waiters
        if ( ready_)
            boost::throw_exception(
                promise_already_satisfied() );
        except_ = except;
        mark_ready_and_notify_();
    }

    void get_( unique_lock< mutex > & lk)
    {
        //TODO: the get method waits until the future has a valid result and
        //      (depending on which template is used) retrieves it
        //      it effectively calls wait_() in order to wait for the result
        //      if it satisfies the requirements of MoveAssignable, the value is moved,
        //      otherwise it is copied
        wait_( lk);
        if ( except_)
            rethrow_exception( except_);
    }

    void wait_( unique_lock< mutex > & lk) const
    {
        //TODO: blocks until the result becomes available
        while ( ! ready_)
            waiters_.wait( lk);
    }

protected:
    virtual void deallocate_future() = 0;

public:
    typedef intrusive_ptr< future_base >    ptr_t;

    future_base() :
        use_count_( 0), mtx_(), ready_( false), except_()
    {}

    virtual ~future_base() {}

    void owner_destroyed()
    {
        //TODO: lock mutex
        //      set broken_exception if future was not already done
        //      done = true, notify all waiters
        unique_lock< mutex > lk( mtx_);
        owner_destroyed_();
    }

    void set_value()
    {
        //TODO: store the value into the shared state and make the state ready
        //      the operation is atomic, i.e. it behaves as though they acquire a single mutex
        //      associated with the promise object while updating the promise object
        //      an exception is thrown if there is no shared state or the shared state already
        //      stores a value or exception
        unique_lock< mutex > lk( mtx_);
        set_value_();
    }

    void set_exception( exception_ptr except)
    {
        //TODO: store the exception pointer p into the shared state and make the state ready
        //      the operation is atomic, i.e. it behaves as though they acquire a single mutex
        //      associated with the promise object while updating the promise object
        //      an exception is thrown if there is no shared state or the shared state already
        //      stores a value or exception
        unique_lock< mutex > lk( mtx_);
        set_exception_( except);
    }

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
        unique_lock< mutex > lk( mtx_);
        get_( lk);
    }

    void wait() const
    {
        //TODO: blocks until the result becomes available
        //      valid() == true after the call
        unique_lock< mutex > lk( mtx_);
        wait_( lk);
    }

    friend inline void intrusive_ptr_add_ref( future_base * p) BOOST_NOEXCEPT
    { ++p->use_count_; }

    friend inline void intrusive_ptr_release( future_base * p)
    {
        if ( 0 == --p->use_count_)
           p->deallocate_future();
    }
};

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DETAIL_FUTURE_BASE_H
