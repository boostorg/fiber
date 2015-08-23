//
// yield.hpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// modified by Oliver Kowalke and Nat Goodspeed
//

#ifndef BOOST_FIBERS_ASIO_YIELD_HPP
#define BOOST_FIBERS_ASIO_YIELD_HPP

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
/// suspend the calling fiber until completion.
/**
 * The yield_t class is used to indicate that a Boost.Asio asynchronous
 * operation should suspend the calling fiber until its completion. The
 * asynchronous function will either return a suitable value, or will throw an
 * exception indicating the error. A yield_t object may be passed as a handler
 * to an asynchronous operation, typically using the special value @c
 * boost::fibers::asio::yield. For example:
 *
 * @code std::size_t length_read
 *   = my_socket.async_read_some(my_buffer, boost::fibers::asio::yield); @endcode
 *
 * The initiating function (async_read_some in the above example) does not
 * return to the calling fiber until the asynchronous read has completed. Like
 * its synchronous counterpart, it returns the result of the operation. If the
 * operation completes with an error_code indicating failure, it is converted
 * into a system_error and thrown as an exception.
 *
 * To suppress a possible error exception:
 * @code
 * boost::system::error_code ec;
 * std::size_t length_read =
 *     my_socket.async_read_some(my_buffer, boost::fibers::asio::yield[ec]);
 * // test ec for success
 * @endcode
 *
 * The crucial distinction between
 * @code
 * std::size_t length_read = my_socket.read_some(my_buffer);
 * @endcode
 * and
 * @code
 * std::size_t length_read =
 *     my_socket.async_read_some(my_buffer, boost::fibers::asio::yield);
 * @code
 * is that <tt>read_some()</tt> blocks the entire calling @em thread, whereas
 * <tt>async_read_some(..., boost::fibers::asio::yield)</tt> blocks only the
 * calling @em fiber, permitting other fibers on the same thread to continue
 * running.
 *
 * To specify an alternate allocator for the internal
 * <tt>boost::fibers::promise</tt>:
 * @code
 * boost::fibers::asio::yield.with(alloc_instance)
 * @endcode
 *
 * To bind a <tt>boost::system::error_code</tt> @a ec as well as using an
 * alternate allocator:
 * @code
 * boost::fibers::asio::yield.with(alloc_instance)[ec]
 * @endcode
 */
template< typename Allocator = std::allocator< void > >
class yield_t: public promise_completion_token<Allocator>
{
public:
    /// Construct with default-constructed allocator.
    BOOST_CONSTEXPR yield_t()
    {}

    /// Construct using specified allocator.
    explicit yield_t( Allocator const& allocator) :
        promise_completion_token<Allocator>( allocator)
    {}

    /// Specify an alternate allocator.
    template< typename OtherAllocator >
    yield_t< OtherAllocator >
    with( OtherAllocator const& allocator) const
    { return yield_t< OtherAllocator >( allocator); }

    /// Bind an error_code to suppress error exception.
    yield_t operator[]( boost::system::error_code & ec) const
    {
        // Return a copy because typical usage will be on our canonical
        // instance. Don't leave the canonical instance with a dangling
        // binding to a transient error_code!
        yield_t tmp;
        tmp.ec_ = & ec;
        return tmp;
    }
};

/// A special value, similar to std::nothrow.
BOOST_CONSTEXPR_OR_CONST yield_t<> yield;

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#include "detail/yield.hpp"

#endif // BOOST_FIBERS_ASIO_YIELD_HPP
