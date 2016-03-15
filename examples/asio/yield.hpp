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

class yield_base {
public:
    constexpr yield_base() = default;

    /**
     * @code
     * static yield_base yield;
     * boost::system::error_code myec;
     * func(yield[myec]);
     * @endcode
     * @c yield[myec] returns an instance of @c yield_base whose @c ec_ points
     * to @c myec. The expression @c yield[myec] "binds" @c myec to that
     * (anonymous) @c yield_base instance, instructing @c func() to store any
     * @c error_code it might produce into @c myec rather than throwing @c
     * boost::system::system_error.
     */
    yield_base operator[]( boost::system::error_code & ec) const {
        yield_base tmp{ *this };
        tmp.ec_ = & ec;
        return tmp;
    }

//private:
    // ptr to bound error_code instance if any
    boost::system::error_code   *   ec_{ nullptr };
    // allow calling fiber to "hop" to another thread if it could resume more
    // quickly that way
    bool                            allow_hop_{ false };
};

class yield_t : public yield_base {
};

class yield_hop_t : public yield_base {
public:
    yield_hop_t() {
        allow_hop_ = true;
    }
};

// canonical instance with allow_hop_ == false
thread_local yield_t yield{};
// canonical instance with allow_hop_ == true
thread_local yield_hop_t yield_hop{};

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#include "detail/yield.hpp"

#endif // BOOST_FIBERS_ASIO_YIELD_HPP
