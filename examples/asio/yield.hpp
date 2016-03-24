//          Copyright 2003-2013 Christopher M. Kohlhoff
//          Copyright Oliver Kowalke, Nat Goodspeed 2015.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)


#ifndef BOOST_FIBERS_ASIO_YIELD_HPP
#define BOOST_FIBERS_ASIO_YIELD_HPP

#include <boost/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace asio {

class yield_t {
public:
    yield_t( bool hop) :
        allow_hop_( hop) {
    }

    /**
     * @code
     * static yield_t yield;
     * boost::system::error_code myec;
     * func(yield[myec]);
     * @endcode
     * @c yield[myec] returns an instance of @c yield_t whose @c ec_ points
     * to @c myec. The expression @c yield[myec] "binds" @c myec to that
     * (anonymous) @c yield_t instance, instructing @c func() to store any
     * @c error_code it might produce into @c myec rather than throwing @c
     * boost::system::system_error.
     */
    yield_t operator[]( boost::system::error_code & ec) const {
        yield_t tmp{ * this };
        tmp.ec_ = & ec;
        return tmp;
    }

//private:
    // ptr to bound error_code instance if any
    boost::system::error_code   *   ec_{ nullptr };
    // allow calling fiber to "hop" to another thread if it could resume more
    // quickly that way
    bool                            allow_hop_;
};

// canonical instance with allow_hop_ == false
thread_local yield_t yield{ false };
// canonical instance with allow_hop_ == true
thread_local yield_t yield_hop{ true };

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#include "detail/yield.hpp"

#endif // BOOST_FIBERS_ASIO_YIELD_HPP
