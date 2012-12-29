
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// based on boost.thread

#ifndef BOOST_fiber_EXCEPTIONS_H
#define BOOST_fiber_EXCEPTIONS_H

#include <stdexcept>
#include <string>

#include <boost/config.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>

#include <boost/fiber/detail/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

class fiber_interrupted
{};

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
        fiber_exception(i ev, "boost::fiber_resource_error")
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
        fiber_eception(
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

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_fiber_EXCEPTIONS_H
