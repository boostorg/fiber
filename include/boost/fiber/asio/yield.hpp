//
// yield.hpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// modified by Oliver Kowalke
//

#ifndef BOOST_FIBERS_ASIO_YIELD_HPP
#define BOOST_FIBERS_ASIO_YIELD_HPP

#include <memory>

#include <boost/asio/detail/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace asio {

class yield_t
{
public:
    BOOST_CONSTEXPR yield_t() :
        ec_( 0)
    {}

    yield_t operator[]( boost::system::error_code & ec) const
    {
        yield_t tmp;
        tmp.ec_ = & ec;
        return tmp;
    }

//private:
    boost::system::error_code   *   ec_;
};

BOOST_CONSTEXPR_OR_CONST yield_t yield;

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#include <boost/fiber/asio/detail/yield.hpp>

#endif // BOOST_FIBERS_ASIO_YIELD_HPP
