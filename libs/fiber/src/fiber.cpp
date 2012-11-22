
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_FIBERS_SOURCE

#include <boost/fiber/fiber.hpp>

#include <boost/assert.hpp>

#include <boost/fiber/detail/scheduler.hpp>
#include <boost/fiber/exceptions.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

void
fiber::cancel()
{
    BOOST_ASSERT( impl_);
    impl_->cancel();
}

bool
fiber::join()
{
    BOOST_ASSERT( impl_);
    if ( ! impl_->is_complete() )
        detail::scheduler::instance().join( impl_);
    BOOST_ASSERT( impl_->is_complete() );
    return ! impl_->is_canceled();
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
