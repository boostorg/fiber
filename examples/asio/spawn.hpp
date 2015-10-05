//
// spawn.hpp
// ~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_FIBERS_ASIO_SPAWN_HPP
#define BOOST_FIBERS_ASIO_SPAWN_HPP

#include <boost/asio/detail/config.hpp>
#include <boost/asio/detail/weak_ptr.hpp>
#include <boost/asio/detail/wrapped_handler.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/strand.hpp>
#include <boost/config.hpp>

#include <boost/fiber/all.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace asio {

/// Context object the represents the currently executing fiber.
/**
 * The basic_yield_context class is used to represent the currently executing
 * fiber. A basic_yield_context may be passed as a handler to an * asynchronous
 * operation. For example:
 *
 * @code template< typename Handler >
 * void my_fiber( basic_yield_context< Handler > yield)
 * {
 *   ...
 *   std::size_t n = my_socket.async_read_some( buffer, yield);
 *   ...
 * } @endcode
 *
 * The initiating function (async_read_some in the above example) suspends the
 * current fiber. The fiber is resumed when the asynchronous operation
 * completes, and the result of the operation is returned.
 */
template< typename Handler >
class basic_yield_context
{
public:
  /// Construct a yield context to represent the specified fiber.
  /**
   * Most applications do not need to use this constructor. Instead, the
   * spawn() function passes a yield context as an argument to the fiber
   * function.
   */
  basic_yield_context(
          boost::fibers::context * ctx,
          Handler& handler) :
      ctx_( ctx),
      handler_( handler),
      ec_( 0)
  {}

  /// Return a yield context that sets the specified error_code.
  /**
   * By default, when a yield context is used with an asynchronous operation, a
   * non-success error_code is converted to system_error and thrown. This
   * operator may be used to specify an error_code object that should instead be
   * set with the asynchronous operation's result. For example:
   *
   * @code template< typename Handler >
   * void my_fiber( basic_yield_context< Handler > yield)
   * {
   *   ...
   *   std::size_t n = my_socket.async_read_some( buffer, yield[ec]);
   *   if ( ec)
   *   {
   *     // An error occurred.
   *   }
   *   ...
   * } @endcode
   */
  basic_yield_context operator[]( boost::system::error_code & ec)
  {
    basic_yield_context tmp( * this);
    tmp.ec_ = & ec;
    return tmp;
  }

#if defined(GENERATING_DOCUMENTATION)
private:
#endif // defined(GENERATING_DOCUMENTATION)
  boost::fibers::context        *   ctx_;
  Handler                       &   handler_;
  boost::system::error_code     *   ec_;
};

#if defined(GENERATING_DOCUMENTATION)
/// Context object the represents the currently executing fiber.
typedef basic_yield_context< unspecified > yield_context;
#else // defined(GENERATING_DOCUMENTATION)
typedef basic_yield_context<
  boost::asio::detail::wrapped_handler<
    boost::asio::io_service::strand, void(*)(),
    boost::asio::detail::is_continuation_if_running> > yield_context;
#endif // defined(GENERATING_DOCUMENTATION)

/**
 * @defgroup spawn boost::fibers::asio::spawn
 *
 * @brief Start a new stackful fiber.
 *
 * The spawn() function is a high-level wrapper over the Boost.Fiber
 * library. This function enables programs to implement asynchronous logic in a
 * synchronous manner, as illustrated by the following example:
 *
 * @code boost::asio::spawn( my_strand, do_echo);
 *
 * // ...
 *
 * void do_echo( boost::fibers::asio::yield_context yield)
 * {
 *   try
 *   {
 *     char data[128];
 *     for (;;)
 *     {
 *       std::size_t length =
 *         my_socket.async_read_some(
 *           boost::asio::buffer( data), yield);
 *
 *       boost::asio::async_write( my_socket,
 *           boost::asio::buffer( data, length), yield);
 *     }
 *   }
 *   catch ( std::exception const& e)
 *   {
 *     // ...
 *   }
 * } @endcode
 */
/*@{*/

/// Start a new fiber, calling the specified handler when it completes.
/**
 * This function is used to launch a new fiber.
 *
 * @param handler A handler to be called when the fiber exits. More
 * importantly, the handler provides an execution context (via the handler
 * invocation hook) for the fiber. The handler must have the signature:
 * @code void handler(); @endcode
 *
 * @param function The fiber function. The function must have the signature:
 * @code void function( basic_yield_context< Handler > yield); @endcode
 *
 */
template< typename Handler, typename Function >
void spawn( boost::asio::io_service & io_service,
        BOOST_ASIO_MOVE_ARG( Handler) handler,
        BOOST_ASIO_MOVE_ARG( Function) function);

/// Start a new fiber, inheriting the execution context of another.
/**
 * This function is used to launch a new fiber.
 *
 * @param ctx Identifies the current fiber as a parent of the new
 * fiber. This specifies that the new fiber should inherit the
 * execution context of the parent. For example, if the parent fiber is
 * executing in a particular strand, then the new fiber will execute in the
 * same strand.
 *
 * @param function The fiber function. The function must have the signature:
 * @code void function( basic_yield_context< Handler > yield); @endcode
 *
 */
template< typename Handler, typename Function >
void spawn( boost::asio::io_service & io_service,
        basic_yield_context< Handler > ctx,
        BOOST_ASIO_MOVE_ARG( Function) function);

/// Start a new fiber that executes in the contex of a strand.
/**
 * This function is used to launch a new fiber.
 *
 * @param strand Identifies a strand. By starting multiple fibers on the
 * same strand, the implementation ensures that none of those fibers can
 * execute simultaneously.
 *
 * @param function The fiber function. The function must have the signature:
 * @code void function( yield_context yield); @endcode
 *
 */
template< typename Function >
void spawn( boost::asio::io_service::strand strand,
        BOOST_ASIO_MOVE_ARG( Function) function);

/// Start a new fiber that executes on a given io_service.
/**
 * This function is used to launch a new fiber.
 *
 * @param io_service Identifies the io_service that will run the fiber. The
 * new fiber is implicitly given its own strand within this io_service.
 *
 * @param function The fiber function. The function must have the signature:
 * @code void function( yield_context yield); @endcode
 *
 */
template< typename Function >
void spawn( boost::asio::io_service & io_service,
        BOOST_ASIO_MOVE_ARG( Function) function);

} // namespace asio
} // namespace fibers
} // namespace boost

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#include "detail/spawn.hpp"

#endif // BOOST_FIBERS_ASIO_SPAWN_HPP
