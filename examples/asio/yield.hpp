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

#include <memory>

#include <boost/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace asio {

class yield_t {
public:
    constexpr yield_t() = default;

    yield_t operator[]( boost::system::error_code & ec) const {
        yield_t tmp;
        tmp.ec_ = & ec;
        return tmp;
    }

//private:
    boost::system::error_code   *   ec_{ nullptr };
};

thread_local yield_t yield{};

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#include "detail/yield.hpp"

#endif // BOOST_FIBERS_ASIO_YIELD_HPP
