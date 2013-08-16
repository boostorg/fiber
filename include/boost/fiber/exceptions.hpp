
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// based on boost.thread

#ifndef BOOST_fiber_EXCEPTIONS_H
#define BOOST_fiber_EXCEPTIONS_H

#include <stdexcept>
#include <string>

#include <boost/config.hpp>
#include <boost/detail/scoped_enum_emulation.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>

#include <boost/fiber/detail/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

class fiber_exception : public system::system_error
{
public:
    fiber_exception() :
        system::system_error( 0, system::system_category() )
    {}

    fiber_exception( int sys_error_code) :
        system::system_error( sys_error_code, system::system_category() )
    {}

    fiber_exception( int ev, const char * what_arg) :
        system::system_error(
            system::error_code( ev, system::system_category() ), what_arg)
    {}

    fiber_exception( int ev, const std::string & what_arg) :
        system::system_error(
            system::error_code( ev, system::system_category() ), what_arg)
    {}

    ~fiber_exception() throw()
    {}

    int native_error() const
    { return code().value(); }
};

class condition_error : public system::system_error
{
public:
    condition_error() :
        system::system_error(
            system::error_code( 0, system::system_category() ), "Condition error")
    {}

    condition_error( int ev) :
        system::system_error(
            system::error_code( ev, system::system_category() ), "Condition error")
    {}

    condition_error( int ev, const char * what_arg) :
        system::system_error(
            system::error_code( ev, system::system_category() ), what_arg)
    {}

    condition_error( int ev, const std::string & what_arg) :
        system::system_error(
            system::error_code( ev, system::system_category() ), what_arg)
    {}
};

class lock_error : public fiber_exception
{
public:
    lock_error() :
        fiber_exception(0, "boost::lock_error")
    {}

    lock_error( int ev ) :
        fiber_exception( ev, "boost::lock_error")
    {}

    lock_error( int ev, const char * what_arg) :
        fiber_exception( ev, what_arg)
    {}

    lock_error( int ev, const std::string & what_arg) :
        fiber_exception( ev, what_arg)
    {}

    ~lock_error() throw()
    {}
};

class fiber_resource_error : public fiber_exception
{
public:
    fiber_resource_error() :
        fiber_exception(
            system::errc::resource_unavailable_try_again,
            "boost::fiber_resource_error")
    {}

    fiber_resource_error( int ev) :
        fiber_exception( ev, "boost::fiber_resource_error")
    {}

    fiber_resource_error( int ev, const char * what_arg) :
        fiber_exception( ev, what_arg)
    {}

    fiber_resource_error( int ev, const std::string & what_arg) :
        fiber_exception( ev, what_arg)
    {}

    ~fiber_resource_error() throw()
    {}
};

class invalid_argument : public fiber_exception
{
public:
    invalid_argument() :
        fiber_exception(
            system::errc::invalid_argument, "boost::invalid_argument")
    {}

    invalid_argument( int ev) :
        fiber_exception( ev, "boost::invalid_argument")
    {}

    invalid_argument( int ev, const char * what_arg) :
        fiber_exception( ev, what_arg)
    {}

    invalid_argument( int ev, const std::string & what_arg) :
        fiber_exception( ev, what_arg)
    {}
};

class fiber_interrupted : public fiber_exception
{
public:
    fiber_interrupted() :
        fiber_exception(
            system::errc::interrupted, "boost::fiber_interrupted")
    {}
};

BOOST_SCOPED_ENUM_DECLARE_BEGIN(future_errc)
{
    broken_promise = 1,
    future_already_retrieved,
    promise_already_satisfied,
    no_state
}
BOOST_SCOPED_ENUM_DECLARE_END(future_errc)

BOOST_FIBERS_DECL system::error_category const& future_category() BOOST_NOEXCEPT;

}

namespace system {

template<>
struct is_error_code_enum< fibers::future_errc > : public true_type
{};

#ifdef BOOST_NO_CXX11_SCOPED_ENUMS
template<>
struct is_error_code_enum< fibers::future_errc::enum_type > : public true_type
{};
#endif

inline
error_code make_error_code( fibers::future_errc e) //BOOST_NOEXCEPT
{
    return error_code( underlying_cast< int >( e), fibers::future_category() );
}

inline
error_condition make_error_condition( fibers::future_errc e) //BOOST_NOEXCEPT
{
    return error_condition( underlying_cast< int >( e), fibers::future_category() );
}

}

namespace fibers {

class future_error : public std::logic_error
{
private:
    system::error_code  ec_;

public:
    future_error( system::error_code ec) :
        logic_error( ec.message() ),
        ec_( ec)
    {}

    system::error_code const& code() const BOOST_NOEXCEPT
    { return ec_; }

    const char* what() const throw()
    { return code().message().c_str(); }
};

class future_uninitialized : public future_error
{
public:
    future_uninitialized() :
        future_error(
            system::make_error_code(
                future_errc::no_state) )
    {}
};

class future_already_retrieved : public future_error
{
public:
    future_already_retrieved() :
        future_error(
            system::make_error_code(
                future_errc::future_already_retrieved) )
    {}
};

class broken_promise : public future_error
{
public:
    broken_promise() :
        future_error(
            system::make_error_code(
                future_errc::broken_promise) )
    {}
};

class promise_already_satisfied : public future_error
{
public:
    promise_already_satisfied() :
        future_error(
            system::make_error_code(
                future_errc::promise_already_satisfied) )
    {}
};

class promise_uninitialized : public future_error
{
public:
    promise_uninitialized() :
        future_error(
            system::make_error_code(
                future_errc::no_state) )
    {}
};

class packaged_task_uninitialized : public future_error
{
public:
    packaged_task_uninitialized() :
        future_error(
            system::make_error_code(
                future_errc::no_state) )
    {}
};

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_fiber_EXCEPTIONS_H
