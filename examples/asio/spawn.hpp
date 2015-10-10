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

#include <boost/fiber/context.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace asio {

template< typename Handler >
class basic_yield_context {
public:
  basic_yield_context(
          boost::fibers::context * ctx,
          Handler& handler) :
      ctx_( ctx),
      handler_( handler),
      ec_( 0) {
  }

  basic_yield_context operator[]( boost::system::error_code & ec) {
      basic_yield_context tmp( * this);
      tmp.ec_ = & ec;
      return tmp;
  }

private:
  boost::fibers::context        *   ctx_;
  Handler                       &   handler_;
  boost::system::error_code     *   ec_;
};

typedef basic_yield_context<
  boost::asio::detail::wrapped_handler<
    boost::asio::io_service::strand, void(*)(),
    boost::asio::detail::is_continuation_if_running> > yield_context;

template< typename Handler, typename Function >
void spawn( boost::asio::io_service & io_service,
        BOOST_ASIO_MOVE_ARG( Handler) handler,
        BOOST_ASIO_MOVE_ARG( Function) function);

template< typename Handler, typename Function >
void spawn( boost::asio::io_service & io_service,
        basic_yield_context< Handler > ctx,
        BOOST_ASIO_MOVE_ARG( Function) function);

template< typename Function >
void spawn( boost::asio::io_service::strand strand,
        BOOST_ASIO_MOVE_ARG( Function) function);

template< typename Function >
void spawn( boost::asio::io_service & io_service,
        BOOST_ASIO_MOVE_ARG( Function) function);

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#include "detail/spawn.hpp"

#endif // BOOST_FIBERS_ASIO_SPAWN_HPP
