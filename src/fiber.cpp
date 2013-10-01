
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/fiber/fiber.hpp"

#include <boost/assert.hpp>
#include <boost/exception/all.hpp>
#include <boost/scope_exit.hpp>
#include <boost/system/error_code.hpp>

#include "boost/fiber/detail/scheduler.hpp"
#include "boost/fiber/exceptions.hpp"
#include "boost/fiber/operations.hpp"

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

void
fiber::start_fiber_()
{
    detail::scheduler::instance()->spawn( impl_);
    // check if joined fiber was interrupted
    if ( impl_->has_exception() )
        impl_->rethrow();
}

int
fiber::priority() const BOOST_NOEXCEPT
{
    BOOST_ASSERT( impl_);

    return impl_->priority();
}

void
fiber::priority( int prio) BOOST_NOEXCEPT
{
    BOOST_ASSERT( impl_);

    detail::scheduler::instance()->priority( impl_, prio);
}

void
fiber::join()
{
    BOOST_ASSERT( impl_);

    if ( boost::this_fiber::get_id() == get_id() )
        boost::throw_exception(
            fiber_resource_error(
                system::errc::resource_deadlock_would_occur, "boost fiber: trying joining itself") );

    if ( ! joinable() )
    {
        boost::throw_exception(
            fiber_resource_error(
                system::errc::invalid_argument, "boost fiber: fiber not joinable") );
    }

    detail::scheduler::instance()->join( impl_);

    ptr_t tmp;
    tmp.swap( impl_);
    // check if joined fiber was interrupted
    if ( tmp->has_exception() )
        tmp->rethrow();
}

void
fiber::detach() BOOST_NOEXCEPT
{
    BOOST_ASSERT( impl_);

    if ( ! joinable() )
    {
        boost::throw_exception(
            fiber_resource_error(
                system::errc::invalid_argument, "boost fiber: fiber not joinable") );
    }

    impl_.reset();
}

void
fiber::interrupt() BOOST_NOEXCEPT
{
    BOOST_ASSERT( impl_);

    impl_->request_interruption( true);
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
