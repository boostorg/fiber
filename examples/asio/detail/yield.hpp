//
// yield.hpp
// ~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// modified by Oliver Kowalke and Nat Goodspeed
//

#ifndef BOOST_FIBERS_ASIO_DETAIL_YIELD_HPP
#define BOOST_FIBERS_ASIO_DETAIL_YIELD_HPP

#include <boost/asio/async_result.hpp>
#include <boost/asio/handler_type.hpp>

#include <boost/fiber/all.hpp>

#include "promise_handler.hpp"

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace asio {
namespace detail {

// yield_handler is just an alias for promise_handler -- but we must
// distinguish this case to specialize async_result below.
//[fibers_asio_yield_handler
template< typename T >
using yield_handler = promise_handler< T >;
//]

}}}

namespace asio {

// Handler traits specialisation for yield_handler.
template< typename T >
class async_result< fibers::asio::detail::yield_handler< T > > {
public:
    // The initiating function will return a value of type T.
    typedef T   type;

    // Constructor creates a new promise for the async operation, and obtains the
    // corresponding future.
    explicit async_result( fibers::asio::detail::yield_handler< T > & h) {
        future_ = h.get_promise()->get_future();
    }
    
    // This blocks the calling fiber until the handler sets either a value or
    // an exception.
    type get() {
        return future_.get();
    }

private:
    fibers::future< T >     future_;
};

// Handler type specialisation for yield for a nullary callback.
template< typename Allocator, typename ReturnType >
struct handler_type< boost::fibers::asio::yield_t< Allocator >,
                     ReturnType() > {
    typedef boost::fibers::asio::detail::yield_handler< void >    type;
};

// Handler type specialisation for yield for a single-argument callback.
template< typename Allocator, typename ReturnType, typename Arg1 >
struct handler_type< boost::fibers::asio::yield_t< Allocator >,
                     ReturnType( Arg1) > {
    typedef fibers::asio::detail::yield_handler< Arg1 >    type;
};

// Handler type specialisation for yield for a callback passed only
// boost::system::error_code. Note the use of yield_handler<void>: an
// error_code indicating error will be conveyed to consumer code via an
// exception. Normal return implies (! error_code).
template< typename Allocator, typename ReturnType >
struct handler_type< boost::fibers::asio::yield_t< Allocator >,
                     ReturnType( boost::system::error_code) > {
    typedef fibers::asio::detail::yield_handler< void >    type;
};

// Handler type specialisation for yield for a callback passed
// boost::system::error_code plus an arbitrary value. Note the use of a
// single-argument yield_handler: an error_code indicating error will be
// conveyed to consumer code via an exception. Normal return implies (!
// error_code).
//[asio_handler_type
template< typename Allocator, typename ReturnType, typename Arg2 >
struct handler_type< boost::fibers::asio::yield_t< Allocator >,
                     ReturnType( boost::system::error_code, Arg2) > {
    typedef fibers::asio::detail::yield_handler< Arg2 >    type;
};
//]

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_ASIO_DETAIL_YIELD_HPP
