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

#ifndef BOOST_FIBERS_ASIO_USE_FUTURE_HPP
#define BOOST_FIBERS_ASIO_USE_FUTURE_HPP

#include <memory>                   // std::allocator
#include <boost/config.hpp>
#include "promise_completion_token.hpp"

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace asio {

/// Class used to specify that a Boost.Asio asynchronous operation should
/// return a future.
/**
 * The use_future_t class is used to indicate that a Boost.Asio asynchronous
 * operation should return a boost::fibers::future object. A use_future_t
 * object may be passed as a handler to an asynchronous operation, typically
 * using the special value @c boost::fibers::asio::use_future. For example:
 *
 * @code boost::fibers::future<std::size_t> my_future
 *   = my_socket.async_read_some(my_buffer, boost::fibers::asio::use_future); @endcode
 *
 * The initiating function (async_read_some in the above example) returns a
 * future that will receive the result of the operation. If the operation
 * completes with an error_code indicating failure, it is converted into a
 * system_error and passed back to the caller via the future.
 */
template< typename Allocator = std::allocator< void > >
class use_future_t : public promise_completion_token< Allocator > {
public:
    /// Construct using default-constructed allocator.
    BOOST_CONSTEXPR use_future_t() {
    }

    /// Construct using specified allocator.
    explicit use_future_t( Allocator const& allocator) :
        promise_completion_token<Allocator>( allocator) {
    }

    /// Specify an alternate allocator.
    template< typename OtherAllocator >
    use_future_t< OtherAllocator >
    operator[]( OtherAllocator const& allocator) const {
        return use_future_t< OtherAllocator >( allocator);
    }
};

/// A special value, similar to std::nothrow.
BOOST_CONSTEXPR_OR_CONST use_future_t<> use_future;

}}} // namespace asio

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#include "detail/use_future.hpp"

#endif // BOOST_FIBERS_ASIO_USE_FUTURE_HPP
