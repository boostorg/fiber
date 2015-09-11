
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/fiber/fiber.hpp"

#include <system_error>

#include <boost/assert.hpp>

#include "boost/fiber/context.hpp"
#include "boost/fiber/exceptions.hpp"
#include "boost/fiber/operations.hpp"

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

void
fiber::start_() {
    impl_->set_ready();
    context::active()->do_spawn( * this);
}

void
fiber::join() {
    BOOST_ASSERT( impl_);

    if ( boost::this_fiber::get_id() == get_id() ) {
        throw fiber_resource_error( static_cast< int >( std::errc::resource_deadlock_would_occur),
                                    "boost fiber: trying to join itself");
    }

    if ( ! joinable() ) {
        throw fiber_resource_error( static_cast< int >( std::errc::invalid_argument),
                                    "boost fiber: fiber not joinable");
    }

    context::active()->do_join( impl_.get() );

    // check if joined fiber was interrupted
    std::exception_ptr except( impl_->get_exception() );

    ptr_t tmp;
    tmp.swap( impl_);

    // re-throw excpetion
    if ( except) {
        std::rethrow_exception( except);
    }
}

void
fiber::detach() {
    BOOST_ASSERT( impl_);

    if ( ! joinable() ) {
        throw fiber_resource_error( static_cast< int >( std::errc::invalid_argument),
                                    "boost fiber: fiber not joinable");
    }

    ptr_t tmp;
    tmp.swap( impl_);
}

void
fiber::interrupt() noexcept {
    BOOST_ASSERT( impl_);

    impl_->request_interruption( true);
    context::active()->do_signal( impl_.get() );
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
