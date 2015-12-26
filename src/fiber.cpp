
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/fiber/fiber.hpp"

#include <system_error>

#include <boost/assert.hpp>

#include "boost/fiber/exceptions.hpp"
#include "boost/fiber/interruption.hpp"
#include "boost/fiber/scheduler.hpp"

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

void
fiber::start_() noexcept {
    context::active()->attach( impl_.get() );
    context::active()->get_scheduler()->set_ready( impl_.get() );
}

void
fiber::join() {
    // FIXME: must fiber::join() be synchronized?
    if ( context::active()->get_id() == get_id() ) {
        throw fiber_resource_error( std::make_error_code( std::errc::resource_deadlock_would_occur),
                                    "boost fiber: trying to join itself");
    }
    if ( ! joinable() ) {
        throw fiber_resource_error( std::make_error_code( std::errc::invalid_argument),
                                    "boost fiber: fiber not joinable");
    }

    impl_->join();
    impl_.reset();
}

void
fiber::interrupt() noexcept {
    BOOST_ASSERT( nullptr != impl_.get() );
    impl_->request_interruption( true);
    context::active()->set_ready( impl_.get() );
}

void
fiber::detach() {
    if ( ! joinable() ) {
        throw fiber_resource_error( std::make_error_code( std::errc::invalid_argument),
                                    "boost fiber: fiber not joinable");
    }
    impl_.reset();
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
