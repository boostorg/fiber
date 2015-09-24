
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// based on boost.thread

#ifndef BOOST_fiber_EXCEPTIONS_H
#define BOOST_fiber_EXCEPTIONS_H

#include <stdexcept>
#include <string>
#include <system_error>

#include <boost/config.hpp>

#include <boost/fiber/detail/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

struct forced_unwind {};

class fiber_exception : public std::system_error {
public:
    fiber_exception() :
        std::system_error( 0, std::system_category() ) {
    }

    fiber_exception( int sys_error_code) :
        std::system_error( sys_error_code, std::system_category() ) {
    }

    fiber_exception( int ev, const char * what_arg) :
        std::system_error( std::error_code( ev, std::system_category() ), what_arg) {
    }

    fiber_exception( int ev, const std::string & what_arg) :
        std::system_error( std::error_code( ev, std::system_category() ), what_arg) {
    }

    virtual ~fiber_exception() throw() {
    }
};

class condition_error : public fiber_exception {
public:
    condition_error() :
        fiber_exception( 0, "Condition error") {
    }

    condition_error( int ev) :
        fiber_exception( ev, "Condition error") {
    }

    condition_error( int ev, const char * what_arg) :
        fiber_exception( ev, what_arg) {
    }

    condition_error( int ev, const std::string & what_arg) :
        fiber_exception( ev, what_arg) {
    }
};

class lock_error : public fiber_exception {
public:
    lock_error() :
        fiber_exception( 0, "boost::lock_error") {
    }

    lock_error( int ev) :
        fiber_exception( ev, "boost::lock_error") {
    }

    lock_error( int ev, const char * what_arg) :
        fiber_exception( ev, what_arg) {
    }

    lock_error( int ev, const std::string & what_arg) :
        fiber_exception( ev, what_arg) {
    }
};

class fiber_resource_error : public fiber_exception {
public:
    fiber_resource_error() :
        fiber_exception(
            static_cast< int >( std::errc::resource_unavailable_try_again),
            "boost::fiber_resource_error") {
    }

    fiber_resource_error( int ev) :
        fiber_exception( ev, "boost::fiber_resource_error") {
    }

    fiber_resource_error( int ev, const char * what_arg) :
        fiber_exception( ev, what_arg) {
    }

    fiber_resource_error( int ev, const std::string & what_arg) :
        fiber_exception( ev, what_arg) {
    }
};

class invalid_argument : public fiber_exception {
public:
    invalid_argument() :
        fiber_exception(
            static_cast< int >( std::errc::invalid_argument),
            "boost::invalid_argument") {
    }

    invalid_argument( int ev) :
        fiber_exception( ev, "boost::invalid_argument") {
    }

    invalid_argument( int ev, const char * what_arg) :
        fiber_exception( ev, what_arg) {
    }

    invalid_argument( int ev, const std::string & what_arg) :
        fiber_exception( ev, what_arg) {
    }
};

class logic_error : public fiber_exception {
public:
    logic_error() :
        fiber_exception( 0, "boost::logic_error") {
    }

    logic_error( const char * what_arg) :
        fiber_exception( 0, what_arg) {
    }

    logic_error( int ev) :
        fiber_exception( ev, "boost::logic_error") {
    }

    logic_error( int ev, const char * what_arg) :
        fiber_exception( ev, what_arg) {
    }

    logic_error( int ev, const std::string & what_arg) :
        fiber_exception( ev, what_arg) {
    }
};

class fiber_interrupted : public fiber_exception {
public:
    fiber_interrupted() :
        fiber_exception(
            static_cast< int >( std::errc::interrupted),
            "boost::fiber_interrupted") {
    }
};

enum class future_errc {
    unknown = 0,
    broken_promise,
    future_already_retrieved,
    promise_already_satisfied,
    no_state
};

BOOST_FIBERS_DECL
std::error_category const& future_category() noexcept;

}}

namespace std {

template<>
struct is_error_code_enum< boost::fibers::future_errc > : public true_type {
};

inline
std::error_code make_error_code( boost::fibers::future_errc e) noexcept {
    return std::error_code( static_cast< int >( e), boost::fibers::future_category() );
}

inline
std::error_condition make_error_condition( boost::fibers::future_errc e) noexcept {
    return std::error_condition( static_cast< int >( e), boost::fibers::future_category() );
}

}

namespace boost {
namespace fibers {

class future_error : public std::logic_error {
private:
    std::error_code  ec_;

public:
    future_error( std::error_code ec) :
        logic_error( ec.message() ),
        ec_( ec) {
    }

    std::error_code const& code() const noexcept {
        return ec_;
    }

    const char* what() const throw() {
        return code().message().c_str();
    }
};

class future_uninitialized : public future_error {
public:
    future_uninitialized() :
        future_error( std::make_error_code( future_errc::no_state) ) {
    }
};

class future_already_retrieved : public future_error {
public:
    future_already_retrieved() :
        future_error( std::make_error_code( future_errc::future_already_retrieved) ) {
    }
};

class broken_promise : public future_error {
public:
    broken_promise() :
        future_error( std::make_error_code( future_errc::broken_promise) ) {
    }
};

class promise_already_satisfied : public future_error {
public:
    promise_already_satisfied() :
        future_error( std::make_error_code( future_errc::promise_already_satisfied) ) {
    }
};

class promise_uninitialized : public future_error {
public:
    promise_uninitialized() :
        future_error( std::make_error_code( future_errc::no_state) ) {
    }
};

class packaged_task_uninitialized : public future_error {
public:
    packaged_task_uninitialized() :
        future_error( std::make_error_code( future_errc::no_state) ) {
    }
};

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_fiber_EXCEPTIONS_H
