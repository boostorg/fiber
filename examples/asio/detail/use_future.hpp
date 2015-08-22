//
// use_future.hpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// modified by Oliver Kowalke and Nat Goodspeed
//

#ifndef BOOST_FIBERS_ASIO_DETAIL_USE_FUTURE_HPP
#define BOOST_FIBERS_ASIO_DETAIL_USE_FUTURE_HPP

#include <boost/asio/async_result.hpp>
#include <boost/asio/handler_type.hpp>
#include <boost/asio/handler_invoke_hook.hpp>

#include <boost/fiber/all.hpp>

#include "promise_handler.hpp"

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace asio {
namespace detail {

// use_future_handler is just an alias for promise_handler -- but we must
// distinguish this case to specialize async_result below.
template < typename T >
using use_future_handler = promise_handler<T>;

} // detail
} // asio
} // fibers

namespace asio {

// Handler traits specialisation for use_future_handler.
template< typename T >
class async_result< fibers::asio::detail::use_future_handler< T > >
{
public:
    // The initiating function will return a future.
    typedef boost::fibers::future< T >  type;

    // Constructor creates a new promise for the async operation, and obtains the
    // corresponding future.
    explicit async_result( fibers::asio::detail::use_future_handler< T > & h)
    { value_ = h.get_promise()->get_future(); }

    // Obtain the future to be returned from the initiating function.
    type get()
    { return boost::move( value_); }

private:
    type    value_;
};

// Handler type specialisation for use_future for a nullary callback.
template< typename Allocator, typename ReturnType >
struct handler_type<
    boost::fibers::asio::use_future_t< Allocator>,
    ReturnType()
>
{ typedef fibers::asio::detail::use_future_handler< void >   type; };

// Handler type specialisation for use_future for a single-argument callback.
template< typename Allocator, typename ReturnType, typename Arg1 >
struct handler_type<
    boost::fibers::asio::use_future_t< Allocator >,
    ReturnType( Arg1)
>
{ typedef fibers::asio::detail::use_future_handler< Arg1 > type; };

// Handler type specialisation for use_future for a callback passed only
// boost::system::error_code. Note the use of use_future_handler<void>: an
// error_code indicating error will be conveyed to consumer code via
// set_exception().
template< typename Allocator, typename ReturnType >
struct handler_type<
    boost::fibers::asio::use_future_t< Allocator >,
    ReturnType( boost::system::error_code)
>
{ typedef fibers::asio::detail::use_future_handler< void >   type; };

// Handler type specialisation for use_future for a callback passed
// boost::system::error_code plus an arbitrary value. Note the use of a
// single-argument use_future_handler: an error_code indicating error will be
// conveyed to consumer code via set_exception().
template< typename Allocator, typename ReturnType, typename Arg2 >
struct handler_type<
    boost::fibers::asio::use_future_t< Allocator >,
    ReturnType( boost::system::error_code, Arg2)
>
{ typedef fibers::asio::detail::use_future_handler< Arg2 >   type; };

} // namespace asio
} // namespace boost

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_ASIO_DETAIL_USE_FUTURE_HPP
