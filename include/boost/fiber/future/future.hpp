
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_FUTURE_HPP
#define BOOST_FIBERS_FUTURE_HPP

#include <algorithm> // std::move()
#include <chrono>
#include <exception>

#include <boost/config.hpp>

#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/exceptions.hpp>
#include <boost/fiber/future/detail/shared_state.hpp>
#include <boost/fiber/future/future_status.hpp>

namespace boost {
namespace fibers {

template< typename R >
class packaged_task;

template< typename R >
class promise;

template< typename R >
class shared_future;

template< typename R >
class future {
private:
    typedef typename detail::shared_state< R >::ptr_t   ptr_t;

    friend class shared_future< R >;

    ptr_t           state_;

public:
    future() noexcept :
        state_() {
    }

    explicit future( ptr_t const& p) noexcept :
        state_( p) {
    }

    ~future() noexcept {
    }

    future( future const&) = delete;
    future & operator=( future const&) = delete;

    future( future< R > && other) noexcept :
        state_( std::move( other.state_) ) {
    }

    future & operator=( future< R > && other) noexcept {
        if ( this != & other) {
            state_ = std::move( other.state_);
        }
        return * this;
    }

    bool valid() const noexcept {
        return nullptr != state_.get();
    }

    shared_future< R > share();

    R get() {
        if ( ! valid() ) {
            throw future_uninitialized();
        }
        ptr_t tmp;
        tmp.swap( state_);
        return tmp->get();
    }

    std::exception_ptr get_exception_ptr() {
        if ( ! valid() ) {
            throw future_uninitialized();
        }
        return state_->get_exception_ptr();
    }

    void wait() const {
        if ( ! valid() ) {
            throw future_uninitialized();
        }
        state_->wait();
    }

    template< class Rep, class Period >
    future_status wait_for( std::chrono::duration< Rep, Period > const& timeout_duration) const {
        if ( ! valid() ) {
            throw future_uninitialized();
        }
        return state_->wait_for( timeout_duration);
    }

    future_status wait_until( std::chrono::steady_clock::time_point const& timeout_time) const {
        if ( ! valid() ) {
            throw future_uninitialized();
        }
        return state_->wait_until( timeout_time);
    }

    template< typename Clock, typename Duration >
    future_status wait_until( std::chrono::time_point< Clock, Duration > const& timeout_time_) const {
        std::chrono::steady_clock::time_point timeout_time( detail::convert_tp( timeout_time_) );
        return wait_until( timeout_time);
    }
};

template< typename R >
class future< R & > {
private:
    typedef typename detail::shared_state< R & >::ptr_t   ptr_t;

    friend class shared_future< R & >;

    ptr_t           state_;

public:
    future() noexcept :
        state_() {
    }

    explicit future( ptr_t const& p) noexcept :
        state_( p) {
    }

    ~future() noexcept {
    }

    future( future const&) = delete;
    future & operator=( future const&) = delete;

    future( future< R & > && other) noexcept :
        state_( std::move( other.state_) ) {
    }

    future & operator=( future< R & > && other) noexcept {
        if ( this != & other) {
            state_ = std::move( other.state_);
        }
        return * this;
    }

    bool valid() const noexcept {
        return nullptr != state_.get();
    }

    shared_future< R & > share();

    R & get() {
        if ( ! valid() ) {
            throw future_uninitialized();
        }
        ptr_t tmp;
        tmp.swap( state_);
        return tmp->get();
    }

    std::exception_ptr get_exception_ptr() {
        if ( ! valid() ) {
            throw future_uninitialized();
        }
        return state_->get_exception_ptr();
    }

    void wait() const {
        if ( ! valid() ) {
            throw future_uninitialized();
        }
        state_->wait();
    }

    template< class Rep, class Period >
    future_status wait_for( std::chrono::duration< Rep, Period > const& timeout_duration) const {
        if ( ! valid() ) {
            throw future_uninitialized();
        }
        return state_->wait_for( timeout_duration);
    }

    future_status wait_until( std::chrono::steady_clock::time_point const& timeout_time) const {
        if ( ! valid() ) {
            throw future_uninitialized();
        }
        return state_->wait_until( timeout_time);
    }

    template< typename Clock, typename Duration >
    future_status wait_until( std::chrono::time_point< Clock, Duration > const& timeout_time_) const {
        std::chrono::steady_clock::time_point timeout_time( detail::convert_tp( timeout_time_) );
        return wait_until( timeout_time);
    }
};

template<>
class future< void > {
private:
    typedef detail::shared_state< void >::ptr_t   ptr_t;

    friend class shared_future< void >;

    ptr_t           state_;

public:
    future() noexcept :
        state_() {
    }

    explicit future( ptr_t const& p) noexcept :
        state_( p) {
    }

    ~future() noexcept {
    }

    future( future const&) = delete;
    future & operator=( future const&) = delete;

    inline
    future( future< void > && other) noexcept :
        state_( std::move( other.state_) ) {
    }

    inline
    future & operator=( future< void > && other) noexcept {
        if ( this != & other) {
            state_ = std::move( other.state_);
        }
        return * this;
    }

    inline
    bool valid() const noexcept {
        return nullptr != state_.get();
    }

    shared_future< void > share();

    inline
    void get() {
        if ( ! valid() ) {
            throw future_uninitialized();
        }
        ptr_t tmp;
        tmp.swap( state_);
        tmp->get();
    }

    inline
    std::exception_ptr get_exception_ptr() {
        if ( ! valid() ) {
            throw future_uninitialized();
        }
        return state_->get_exception_ptr();
    }

    inline
    void wait() const {
        if ( ! valid() ) {
            throw future_uninitialized();
        }
        state_->wait();
    }

    template< class Rep, class Period >
    future_status wait_for( std::chrono::duration< Rep, Period > const& timeout_duration) const {
        if ( ! valid() ) {
            throw future_uninitialized();
        }
        return state_->wait_for( timeout_duration);
    }

    inline
    future_status wait_until( std::chrono::steady_clock::time_point const& timeout_time) const {
        if ( ! valid() ) {
            throw future_uninitialized();
        }
        return state_->wait_until( timeout_time);
    }

    template< typename Clock, typename Duration >
    future_status wait_until( std::chrono::time_point< Clock, Duration > const& timeout_time_) const {
        std::chrono::steady_clock::time_point timeout_time( detail::convert_tp( timeout_time_) );
        return wait_until( timeout_time);
    }
};


template< typename R >
class shared_future {
private:
    typedef typename detail::shared_state< R >::ptr_t   ptr_t;

    friend class future< R >;

    ptr_t           state_;

    explicit shared_future( ptr_t const& p) noexcept :
        state_( p) {
    }

public:
    shared_future() noexcept :
        state_() {
    }

    ~shared_future() noexcept {
    }

    shared_future( shared_future const& other) :
        state_( other.state_) {
    }

    shared_future( shared_future && other) noexcept :
        state_( std::move( other.state_) ) {
    }

    shared_future( future< R > && other) noexcept :
        state_( std::move( other.state_) ) {
    }

    shared_future & operator=( shared_future const& other) noexcept {
        if ( this != & other) {
            state_ = other.state_;
        }
        return * this;
    }

    shared_future & operator=( shared_future && other) noexcept {
        if ( this != & other) {
            state_= std::move( other.state_);
        }
        return * this;
    }

    shared_future & operator=( future< R > && other) noexcept {
        state_ = std::move( other.state_);
        return * this;
    }

    bool valid() const noexcept {
        return nullptr != state_.get();
    }

    R const& get() const {
        if ( ! valid() ) {
            throw future_uninitialized();
        }
        return state_->get();
    }

    std::exception_ptr get_exception_ptr() {
        if ( ! valid() ) {
            throw future_uninitialized();
        }
        return state_->get_exception_ptr();
    }

    void wait() const {
        if ( ! valid() ) {
            throw future_uninitialized();
        }
        state_->wait();
    }

    template< class Rep, class Period >
    future_status wait_for( std::chrono::duration< Rep, Period > const& timeout_duration) const {
        if ( ! valid() ) {
            throw future_uninitialized();
        }
        return state_->wait_for( timeout_duration);
    }

    future_status wait_until( std::chrono::steady_clock::time_point const& timeout_time) const {
        if ( ! valid() ) {
            throw future_uninitialized();
        }
        return state_->wait_until( timeout_time);
    }

    template< typename Clock, typename Duration >
    future_status wait_until( std::chrono::time_point< Clock, Duration > const& timeout_time_) const {
        std::chrono::steady_clock::time_point timeout_time( detail::convert_tp( timeout_time_) );
        return wait_until( timeout_time);
    }
};

template< typename R >
class shared_future< R & > {
private:
    typedef typename detail::shared_state< R & >::ptr_t   ptr_t;

    friend class future< R & >;

    ptr_t           state_;

    explicit shared_future( ptr_t const& p) noexcept :
        state_( p) {
    }

public:
    shared_future() noexcept :
        state_() {
    }

    ~shared_future() noexcept {
    }

    shared_future( shared_future const& other) :
        state_( other.state_) {
    }

    shared_future( shared_future && other) noexcept :
        state_( std::move( other.state_) ) {
    }

    shared_future( future< R & > && other) noexcept :
        state_( std::move( other.state_) ) {
    }

    shared_future & operator=( shared_future const& other) noexcept {
        if ( this != & other) {
            state_ = other.state_;
        }
        return * this;
    }

    shared_future & operator=( shared_future && other) noexcept {
        if ( this != & other) {
            state_ = std::move( other.state_);
        }
        return * this;
    }

    shared_future & operator=( future< R & > && other) noexcept {
        state_ = std::move( other.state_);
        return * this;
    }

    bool valid() const noexcept {
        return nullptr != state_.get();
    }

    R & get() const {
        if ( ! valid() ) {
            throw future_uninitialized();
        }
        return state_->get();
    }

    std::exception_ptr get_exception_ptr() {
        if ( ! valid() ) {
            throw future_uninitialized();
        }
        return state_->get_exception_ptr();
    }

    void wait() const {
        if ( ! valid() ) {
            throw future_uninitialized();
        }
        state_->wait();
    }

    template< class Rep, class Period >
    future_status wait_for( std::chrono::duration< Rep, Period > const& timeout_duration) const {
        if ( ! valid() ) {
            throw future_uninitialized();
        }
        return state_->wait_for( timeout_duration);
    }

    future_status wait_until( std::chrono::steady_clock::time_point const& timeout_time) const {
        if ( ! valid() ) {
            throw future_uninitialized();
        }
        return state_->wait_until( timeout_time);
    }

    template< typename Clock, typename Duration >
    future_status wait_until( std::chrono::time_point< Clock, Duration > const& timeout_time_) const {
        std::chrono::steady_clock::time_point timeout_time( detail::convert_tp( timeout_time_) );
        return wait_until( timeout_time);
    }
};

template<>
class shared_future< void > {
private:
    typedef detail::shared_state< void >::ptr_t   ptr_t;

    friend class future< void >;

    ptr_t           state_;

    shared_future( ptr_t const& p) noexcept :
        state_( p) {
    }

public:
    shared_future() noexcept :
        state_() {
    }

    ~shared_future() noexcept {
    }

    inline
    shared_future( shared_future const& other) :
        state_( other.state_) {
    }

    inline
    shared_future( shared_future && other) noexcept :
        state_( std::move( other.state_) ) {
    }

    inline
    shared_future( future< void > && other) noexcept :
        state_( std::move( other.state_) ) {
    }

    inline
    shared_future & operator=( shared_future const& other) noexcept
    {
        if ( this != & other) {
            state_ = other.state_;
        }
        return * this;
    }

    inline
    shared_future & operator=( shared_future && other) noexcept {
        if ( this != & other) {
            state_ = std::move( other.state_);
        }
        return * this;
    }

    inline
    shared_future & operator=( future< void > && other) noexcept {
        state_ = std::move( other.state_);
        return * this;
    }

    inline
    bool valid() const noexcept {
        return nullptr != state_.get();
    }

    inline
    void get() const {
        if ( ! valid() ) {
            throw future_uninitialized();
        }
        state_->get();
    }

    inline
    std::exception_ptr get_exception_ptr() {
        if ( ! valid() ) {
            throw future_uninitialized();
        }
        return state_->get_exception_ptr();
    }

    inline
    void wait() const {
        if ( ! valid() ) {
            throw future_uninitialized();
        }
        state_->wait();
    }

    template< class Rep, class Period >
    future_status wait_for( std::chrono::duration< Rep, Period > const& timeout_duration) const {
        if ( ! valid() ) {
            throw future_uninitialized();
        }
        return state_->wait_for( timeout_duration);
    }

    inline
    future_status wait_until( std::chrono::steady_clock::time_point const& timeout_time) const {
        if ( ! valid() ) {
            throw future_uninitialized();
        }
        return state_->wait_until( timeout_time);
    }

    template< typename Clock, typename Duration >
    future_status wait_until( std::chrono::time_point< Clock, Duration > const& timeout_time_) const {
        std::chrono::steady_clock::time_point timeout_time( detail::convert_tp( timeout_time_) );
        return wait_until( timeout_time);
    }
};


template< typename R >
shared_future< R >
future< R >::share() {
    if ( ! valid() ) {
        throw future_uninitialized();
    }
    return shared_future< R >( std::move( * this) );
}

template< typename R >
shared_future< R & >
future< R & >::share() {
    if ( ! valid() ) {
        throw future_uninitialized();
    }
    return shared_future< R & >( std::move( * this) );
}

inline
shared_future< void >
future< void >::share() {
    if ( ! valid() ) {
        throw future_uninitialized();
    }
    return shared_future< void >( std::move( * this) );
}

}}

#endif
