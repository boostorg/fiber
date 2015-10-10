//
// use_future.hpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// modified by Oliver Kowalke
//

#ifndef BOOST_FIBERS_ASIO_USE_FUTURE_HPP
#define BOOST_FIBERS_ASIO_USE_FUTURE_HPP

#include <memory>

#include <boost/config.hpp>
#include <boost/asio/detail/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace asio {

template< typename Allocator = std::allocator< void > >
class use_future_t {
public:
    typedef Allocator allocator_type;

    /// Construct using default-constructed allocator.
    BOOST_CONSTEXPR use_future_t() {
    }

    /// Construct using specified allocator.
    explicit use_future_t( Allocator const& allocator) :
        allocator_( allocator) {
    }

    /// Specify an alternate allocator.
    template< typename OtherAllocator >
    use_future_t< OtherAllocator > operator[]( OtherAllocator const& allocator) const {
        return use_future_t< OtherAllocator >( allocator);
    }

    /// Obtain allocator.
    allocator_type get_allocator() const {
        return allocator_;
    }

private:
    Allocator allocator_;
};

/// A special value, similar to std::nothrow.
/**
 * See the documentation for boost::asio::use_future_t for a usage example.
 */
BOOST_CONSTEXPR_OR_CONST use_future_t<> use_future;

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#include "detail/use_future.hpp"

#endif // BOOST_FIBERS_ASIO_USE_FUTURE_HPP
