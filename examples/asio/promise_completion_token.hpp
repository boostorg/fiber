//
// promise_completion_token.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// modified by Oliver Kowalke and Nat Goodspeed
//

#ifndef BOOST_FIBERS_ASIO_PROMISE_COMPLETION_TOKEN_HPP
#define BOOST_FIBERS_ASIO_PROMISE_COMPLETION_TOKEN_HPP

#include <boost/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace asio {

/// Common base class for yield_t and use_future_t. See also yield.hpp and
/// use_future.hpp.
/**
 * The awkward name of this class is because it's not intended to be used
 * directly in user code: it's the common base class for a couple of user-
 * facing placeholder classes <tt>yield_t</tt> and <tt>use_future_t</tt>. They
 * share a common handler class <tt>promise_handler</tt>.
 *
 * Each subclass (e.g. <tt>use_future_t</tt>) has a canonical instance
 * (<tt>use_future</tt>). These may be used in the following ways as a
 * Boost.Asio asynchronous operation completion token:
 *
 * <dl>
 * <dt><tt>boost::fibers::asio::use_future</tt></dt>
 * <dd>This is the canonical instance of <tt>use_future_t</tt>, provided
 * solely for convenience. It causes <tt>promise_handler</tt> to allocate its
 * internal <tt>boost::fibers::promise</tt> using a default-constructed
 * default allocator (<tt>std::allocator<void></tt>).</dd>
 * <dt><tt>boost::fibers::asio::use_future::with(alloc_instance)</tt></dt>
 * <dd>This usage specifies an alternate allocator instance
 * <tt>alloc_instance</tt>. It causes <tt>promise_handler</tt> to allocate its
 * internal <tt>boost::fibers::promise</tt> using the specified
 * allocator.</dd>
 * </dl>
 */
//[fibers_asio_promise_completion_token
template< typename Allocator >
class promise_completion_token {
public:
    typedef Allocator allocator_type;

    /// Construct using default-constructed allocator.
    constexpr promise_completion_token() = default;

    /// Construct using specified allocator.
    explicit promise_completion_token( Allocator const& allocator) noexcept :
        ec_{ nullptr },
        allocator_{ allocator } {
    }

    /// Obtain allocator.
    allocator_type get_allocator() const {
        return allocator_;
    }

//private:
    // used by some subclasses to bind an error_code to suppress exceptions
    boost::system::error_code   *   ec_{ nullptr };

private:
    Allocator   allocator_{};
};
//]

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_ASIO_PROMISE_COMPLETION_TOKEN_HPP
