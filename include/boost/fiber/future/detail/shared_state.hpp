
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_DETAIL_SHARED_STATE_H
#define BOOST_FIBERS_DETAIL_SHARED_STATE_H

#include <algorithm> // std::move()
#include <atomic>
#include <chrono>
#include <cstddef>
#include <exception>
#include <mutex> // std::unique_lock

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/optional.hpp>

#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/future/future_status.hpp>
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
class shared_state {
private:
    std::atomic< std::size_t >  use_count_;
    mutable mutex               mtx_;
    mutable condition           waiters_;
    bool                        ready_;
    optional< R >               value_;
    std::exception_ptr          except_;

    void mark_ready_and_notify_() {
        ready_ = true;
        waiters_.notify_all();
    }

    void owner_destroyed_() {
        //TODO: set broken_exception if future was not already done
        //      notify all waiters
        if ( ! ready_) {
            set_exception_( std::make_exception_ptr( broken_promise() ) );
        }
    }

    void set_value_( R const& value) {
        //TODO: store the value and make the future ready
        //      notify all waiters
        if ( ready_) {
            throw promise_already_satisfied();
        }
        value_ = value;
        mark_ready_and_notify_();
    }

    void set_value_( R && value) {
        //TODO: store the value and make the future ready
        //      notify all waiters
        if ( ready_) {
            throw promise_already_satisfied();
        }
        value_ = std::move( value);
        mark_ready_and_notify_();
    }

    void set_exception_( std::exception_ptr except) {
        //TODO: store the exception pointer p into the shared state and make the state ready
        //      done = true, notify all waiters
        if ( ready_) {
            throw promise_already_satisfied();
        }
        except_ = except;
        mark_ready_and_notify_();
    }

    R const& get_( std::unique_lock< mutex > & lk) {
        //TODO: the get method waits until the future has a valid result and
        //      (depending on which template is used) retrieves it
        //      it effectively calls wait_() in order to wait for the result
        //      if it satisfies the requirements of moveAssignable, the value is moved,
        //      otherwise it is copied
        wait_( lk);
        if ( except_) {
            std::rethrow_exception( except_);
        }
        return value_.get();
    }

    std::exception_ptr get_exception_ptr_( std::unique_lock< mutex > & lk) {
        wait_( lk);
        return except_;
    }

    void wait_( std::unique_lock< mutex > & lk) const {
        //TODO: blocks until the result becomes available
        while ( ! ready_) {
            waiters_.wait( lk);
        }
    }

    template< class Rep, class Period >
    future_status wait_for_( std::unique_lock< mutex > & lk,
                             std::chrono::duration< Rep, Period > const& timeout_duration) const {
        //TODO: blocks until the result becomes available or timeout
        while ( ! ready_) {
            cv_status st( waiters_.wait_for( lk, timeout_duration) );
            if ( cv_status::timeout == st && ! ready_) {
                return future_status::timeout;
            }
        }
        return future_status::ready;
    }

    future_status wait_until_( std::unique_lock< mutex > & lk,
                              std::chrono::high_resolution_clock::time_point const& timeout_time) const {
        //TODO: blocks until the result becomes available or timeout
        while ( ! ready_) {
            cv_status st( waiters_.wait_until( lk, timeout_time) );
            if ( cv_status::timeout == st && ! ready_) {
                return future_status::timeout;
            }
        }
        return future_status::ready;
    }

protected:
    virtual void deallocate_future() = 0;

public:
    typedef intrusive_ptr< shared_state >    ptr_t;

    shared_state() :
        use_count_( 0), mtx_(), ready_( false),
        value_(), except_() {
    }

    virtual ~shared_state() noexcept {
    }

    shared_state( shared_state const&) = delete;
    shared_state & operator=( shared_state const&) = delete;

    void owner_destroyed() {
        //TODO: lock mutex
        //      set broken_exception if future was not already done
        //      done = true, notify all waiters
        std::unique_lock< mutex > lk( mtx_);
        owner_destroyed_();
    }

    void set_value( R const& value) {
        //TODO: store the value into the shared state and make the state ready
        //      the operation is atomic, i.e. it behaves as though they acquire a single mutex
        //      associated with the promise object while updating the promise object
        //      an exception is thrown if there is no shared state or the shared state already
        //      stores a value or exception
        std::unique_lock< mutex > lk( mtx_);
        set_value_( value);
    }

    void set_value( R && value) {
        //TODO: store the value into the shared state and make the state ready
        //      rhe operation is atomic, i.e. it behaves as though they acquire a single mutex
        //      associated with the promise object while updating the promise object
        //      an exception is thrown if there is no shared state or the shared state already
        //      stores a value or exception
        std::unique_lock< mutex > lk( mtx_);
        set_value_( std::move( value) );
    }

    void set_exception( std::exception_ptr except) {
        //TODO: store the exception pointer p into the shared state and make the state ready
        //      the operation is atomic, i.e. it behaves as though they acquire a single mutex
        //      associated with the promise object while updating the promise object
        //      an exception is thrown if there is no shared state or the shared state already
        //      stores a value or exception
        std::unique_lock< mutex > lk( mtx_);
        set_exception_( except);
    }

    R const& get() {
        //TODO: the get method waits until the future has a valid result and
        //      (depending on which template is used) retrieves it
        //      it effectively calls wait() in order to wait for the result
        //      the value stored in the shared state
        //      if it satisfies the requirements of moveAssignable, the value is moved,
        //      otherwise it is copied
        //      valid() == false after a call to this method.  
        //      detect the case when valid == false before the call and throw a
        //      future_error with an error condition of future_errc::no_state
        std::unique_lock< mutex > lk( mtx_);
        return get_( lk);
    }

    std::exception_ptr get_exception_ptr() {
        std::unique_lock< mutex > lk( mtx_);
        return get_exception_ptr_( lk);
    }

    void wait() const {
        //TODO: blocks until the result becomes available
        //      valid() == true after the call
        std::unique_lock< mutex > lk( mtx_);
        wait_( lk);
    }

    template< class Rep, class Period >
    future_status wait_for( std::chrono::duration< Rep, Period > const& timeout_duration) const {
        //TODO: blocks until the result becomes available or timeout
        //      valid() == true after the call
        std::unique_lock< mutex > lk( mtx_);
        return wait_for_( lk, timeout_duration);
    }

    future_status wait_until( std::chrono::high_resolution_clock::time_point const& timeout_time) const {
        //TODO: blocks until the result becomes available or timeout
        //      valid() == true after the call
        std::unique_lock< mutex > lk( mtx_);
        return wait_until_( lk, timeout_time);
    }

    void reset() {
        ready_ = false;
    }

    friend inline
    void intrusive_ptr_add_ref( shared_state * p) noexcept {
        ++p->use_count_;
    }

    friend inline
    void intrusive_ptr_release( shared_state * p) {
        if ( 0 == --p->use_count_) {
           p->deallocate_future();
        }
    }
};

template< typename R >
class shared_state< R & > {
private:
    std::atomic< std::size_t >  use_count_;
    mutable mutex               mtx_;
    mutable condition           waiters_;
    bool                        ready_;
    R                       *   value_;
    std::exception_ptr          except_;

    void mark_ready_and_notify_() {
        ready_ = true;
        waiters_.notify_all();
    }

    void owner_destroyed_() {
        //TODO: set broken_exception if future was not already done
        //      notify all waiters
        if ( ! ready_) {
            set_exception_( std::make_exception_ptr( broken_promise() ) );
        }
    }

    void set_value_( R & value) {
        //TODO: store the value and make the future ready
        //      notify all waiters
        if ( ready_) {
            throw promise_already_satisfied();
        }
        value_ = & value;
        mark_ready_and_notify_();
    }

    void set_exception_( std::exception_ptr except) {
        //TODO: store the exception pointer p into the shared state and make the state ready
        //      done = true, notify all waiters
        if ( ready_) {
            throw promise_already_satisfied();
        }
        except_ = except;
        mark_ready_and_notify_();
    }

    R & get_( std::unique_lock< mutex > & lk) {
        //TODO: the get method waits until the future has a valid result and
        //      (depending on which template is used) retrieves it
        //      it effectively calls wait_() in order to wait for the result
        //      if it satisfies the requirements of moveAssignable, the value is moved,
        //      otherwise it is copied
        wait_( lk);
        if ( except_) {
            std::rethrow_exception( except_);
        }
        return * value_;
    }

    std::exception_ptr get_exception_ptr_( std::unique_lock< mutex > & lk) {
        wait_( lk);
        return except_;
    }

    void wait_( std::unique_lock< mutex > & lk) const {
        //TODO: blocks until the result becomes available
        while ( ! ready_) {
            waiters_.wait( lk);
        }
    }

    template< class Rep, class Period >
    future_status wait_for_( std::unique_lock< mutex > & lk,
                             std::chrono::duration< Rep, Period > const& timeout_duration) const {
        //TODO: blocks until the result becomes available or timeout
        while ( ! ready_) {
            cv_status st( waiters_.wait_for( lk, timeout_duration) );
            if ( cv_status::timeout == st && ! ready_) {
                return future_status::timeout;
            }
        }
        return future_status::ready;
    }

    future_status wait_until_( std::unique_lock< mutex > & lk,
                              std::chrono::high_resolution_clock::time_point const& timeout_time) const {
        //TODO: blocks until the result becomes available or timeout
        while ( ! ready_) {
            cv_status st( waiters_.wait_until( lk, timeout_time) );
            if ( cv_status::timeout == st && ! ready_) {
                return future_status::timeout;
            }
        }
        return future_status::ready;
    }

protected:
    virtual void deallocate_future() = 0;

public:
    typedef intrusive_ptr< shared_state >    ptr_t;

    shared_state() :
        use_count_( 0), mtx_(), ready_( false),
        value_( 0), except_() {
    }

    virtual ~shared_state() noexcept {
    }

    shared_state( shared_state const&) = delete;
    shared_state & operator=( shared_state const&) = delete;

    void owner_destroyed() {
        //TODO: lock mutex
        //      set broken_exception if future was not already done
        //      done = true, notify all waiters
        std::unique_lock< mutex > lk( mtx_);
        owner_destroyed_();
    }

    void set_value( R & value) {
        //TODO: store the value into the shared state and make the state ready
        //      the operation is atomic, i.e. it behaves as though they acquire a single mutex
        //      associated with the promise object while updating the promise object
        //      an exception is thrown if there is no shared state or the shared state already
        //      stores a value or exception
        std::unique_lock< mutex > lk( mtx_);
        set_value_( value);
    }

    void set_exception( std::exception_ptr except) {
        //TODO: store the exception pointer p into the shared state and make the state ready
        //      the operation is atomic, i.e. it behaves as though they acquire a single mutex
        //      associated with the promise object while updating the promise object
        //      an exception is thrown if there is no shared state or the shared state already
        //      stores a value or exception
        std::unique_lock< mutex > lk( mtx_);
        set_exception_( except);
    }

    R & get() {
        //TODO: the get method waits until the future has a valid result and
        //      (depending on which template is used) retrieves it
        //      it effectively calls wait() in order to wait for the result
        //      the value stored in the shared state
        //      if it satisfies the requirements of moveAssignable, the value is moved,
        //      otherwise it is copied
        //      valid() == false after a call to this method.
        //      detect the case when valid == false before the call and throw a
        //      future_error with an error condition of future_errc::no_state
        std::unique_lock< mutex > lk( mtx_);
        return get_( lk);
    }

    std::exception_ptr get_exception_ptr() {
        std::unique_lock< mutex > lk( mtx_);
        return get_exception_ptr_( lk);
    }

    void wait() const {
        //TODO: blocks until the result becomes available
        //      valid() == true after the call
        std::unique_lock< mutex > lk( mtx_);
        wait_( lk);
    }

    template< class Rep, class Period >
    future_status wait_for( std::chrono::duration< Rep, Period > const& timeout_duration) const {
        //TODO: blocks until the result becomes available or timeout
        //      valid() == true after the call
        std::unique_lock< mutex > lk( mtx_);
        return wait_for_( lk, timeout_duration);
    }

    future_status wait_until( std::chrono::high_resolution_clock::time_point const& timeout_time) const {
        //TODO: blocks until the result becomes available or timeout
        //      valid() == true after the call
        std::unique_lock< mutex > lk( mtx_);
        return wait_until_( lk, timeout_time);
    }

    void reset() {
        ready_ = false;
    }

    friend inline
    void intrusive_ptr_add_ref( shared_state * p) noexcept {
        ++p->use_count_;
    }

    friend inline
    void intrusive_ptr_release( shared_state * p) {
        if ( 0 == --p->use_count_) {
           p->deallocate_future();
        }
    }
};

template<>
class shared_state< void > {
private:
    std::atomic< std::size_t >  use_count_;
    mutable mutex               mtx_;
    mutable condition           waiters_;
    bool                        ready_;
    std::exception_ptr          except_;

    inline
    void mark_ready_and_notify_() {
        ready_ = true;
        waiters_.notify_all();
    }

    inline
    void owner_destroyed_() {
        //TODO: set broken_exception if future was not already done
        //      notify all waiters
        if ( ! ready_) {
            set_exception_( std::make_exception_ptr( broken_promise() ) );
        }
    }

    inline
    void set_value_() {
        //TODO: store the value and make the future ready
        //      notify all waiters
        if ( ready_) {
            throw promise_already_satisfied();
        }
        mark_ready_and_notify_();
    }

    inline
    void set_exception_( std::exception_ptr except) {
        //TODO: store the exception pointer p into the shared state and make the state ready
        //      done = true, notify all waiters
        if ( ready_) {
            throw promise_already_satisfied();
        }
        except_ = except;
        mark_ready_and_notify_();
    }

    inline
    void get_( std::unique_lock< mutex > & lk) {
        //TODO: the get method waits until the future has a valid result and
        //      (depending on which template is used) retrieves it
        //      it effectively calls wait_() in order to wait for the result
        //      if it satisfies the requirements of moveAssignable, the value is moved,
        //      otherwise it is copied
        wait_( lk);
        if ( except_) {
            std::rethrow_exception( except_);
        }
    }

    inline
    std::exception_ptr get_exception_ptr_( std::unique_lock< mutex > & lk) {
        wait_( lk);
        return except_;
    }

    inline
    void wait_( std::unique_lock< mutex > & lk) const {
        //TODO: blocks until the result becomes available
        while ( ! ready_) {
            waiters_.wait( lk);
        }
    }

    template< class Rep, class Period >
    future_status wait_for_( std::unique_lock< mutex > & lk,
                             std::chrono::duration< Rep, Period > const& timeout_duration) const {
        //TODO: blocks until the result becomes available or timeout
        while ( ! ready_) {
            cv_status st( waiters_.wait_for( lk, timeout_duration) );
            if ( cv_status::timeout == st && ! ready_) {
                return future_status::timeout;
            }
        }
        return future_status::ready;
    }

    inline
    future_status wait_until_( std::unique_lock< mutex > & lk,
                               std::chrono::high_resolution_clock::time_point const& timeout_time) const {
        //TODO: blocks until the result becomes available or timeout
        while ( ! ready_) {
            cv_status st( waiters_.wait_until( lk, timeout_time) );
            if ( cv_status::timeout == st && ! ready_) {
                return future_status::timeout;
            }
        }
        return future_status::ready;
    }

protected:
    virtual void deallocate_future() = 0;

public:
    typedef intrusive_ptr< shared_state >    ptr_t;

    shared_state() :
        use_count_( 0), mtx_(), ready_( false), except_() {
    }

    virtual ~shared_state() noexcept {
    }

    shared_state( shared_state const&) = delete;
    shared_state & operator=( shared_state const&) = delete;

    inline
    void owner_destroyed() {
        //TODO: lock mutex
        //      set broken_exception if future was not already done
        //      done = true, notify all waiters
        std::unique_lock< mutex > lk( mtx_);
        owner_destroyed_();
    }

    inline
    void set_value() {
        //TODO: store the value into the shared state and make the state ready
        //      the operation is atomic, i.e. it behaves as though they acquire a single mutex
        //      associated with the promise object while updating the promise object
        //      an exception is thrown if there is no shared state or the shared state already
        //      stores a value or exception
        std::unique_lock< mutex > lk( mtx_);
        set_value_();
    }

    inline
    void set_exception( std::exception_ptr except) {
        //TODO: store the exception pointer p into the shared state and make the state ready
        //      the operation is atomic, i.e. it behaves as though they acquire a single mutex
        //      associated with the promise object while updating the promise object
        //      an exception is thrown if there is no shared state or the shared state already
        //      stores a value or exception
        std::unique_lock< mutex > lk( mtx_);
        set_exception_( except);
    }

    inline
    void get() {
        //TODO: the get method waits until the future has a valid result and
        //      (depending on which template is used) retrieves it
        //      it effectively calls wait() in order to wait for the result
        //      the value stored in the shared state
        //      if it satisfies the requirements of moveAssignable, the value is moved,
        //      otherwise it is copied //      valid() == false after a call to this method.  
        //      detect the case when valid == false before the call and throw a
        //      future_error with an error condition of future_errc::no_state
        std::unique_lock< mutex > lk( mtx_);
        get_( lk);
    }

    inline
    std::exception_ptr get_exception_ptr() {
        std::unique_lock< mutex > lk( mtx_);
        return get_exception_ptr_( lk);
    }

    inline
    void wait() const {
        //TODO: blocks until the result becomes available
        //      valid() == true after the call
        std::unique_lock< mutex > lk( mtx_);
        wait_( lk);
    }

    template< class Rep, class Period >
    future_status wait_for( std::chrono::duration< Rep, Period > const& timeout_duration) const {
        //TODO: blocks until the result becomes available or timeout
        //      valid() == true after the call
        std::unique_lock< mutex > lk( mtx_);
        return wait_for_( lk, timeout_duration);
    }

    inline
    future_status wait_until( std::chrono::high_resolution_clock::time_point const& timeout_time) const {
        //TODO: blocks until the result becomes available or timeout
        //      valid() == true after the call
        std::unique_lock< mutex > lk( mtx_);
        return wait_until_( lk, timeout_time);
    }

    inline
    void reset() {
        ready_ = false;
    }

    friend inline
    void intrusive_ptr_add_ref( shared_state * p) noexcept {
        ++p->use_count_;
    }

    friend inline
    void intrusive_ptr_release( shared_state * p) {
        if ( 0 == --p->use_count_) {
           p->deallocate_future();
        }
    }
};

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DETAIL_SHARED_STATE_H
