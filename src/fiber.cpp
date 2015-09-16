
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/fiber/fiber.hpp"

#include <system_error>

#include <boost/assert.hpp>

#include "boost/fiber/exceptions.hpp"
#include "boost/fiber/scheduler.hpp"

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

void
fiber::start_() {
    context::active()->get_scheduler()->set_ready( impl_.get() );
}

void
fiber::join() {
    if ( context::active()->get_id() == get_id() ) {
        throw fiber_resource_error( static_cast< int >( std::errc::resource_deadlock_would_occur),
                                    "boost fiber: trying to join itself");
    }

    if ( ! joinable() ) {
        throw fiber_resource_error( static_cast< int >( std::errc::invalid_argument),
                                    "boost fiber: fiber not joinable");
    }

    ptr_t tmp;
    tmp.swap( impl_);

    tmp->join();

    // FIXME: call interruption_point()
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
