
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/fiber/fiber.hpp"

#include <boost/assert.hpp>
#include <boost/exception/all.hpp>
#include <boost/scope_exit.hpp>
#include <boost/system/error_code.hpp>

#include "boost/fiber/exceptions.hpp"
#include "boost/fiber/operations.hpp"

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

void
fiber::start_()
{
    impl_->set_ready();
    fm_spawn( impl_.get() );
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

    fm_priority( impl_.get(), prio);
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

    fm_join( impl_.get() );

    // check if joined fiber was interrupted
    exception_ptr except( impl_->get_exception() );

    ptr_t tmp;
    std::swap( tmp, impl_);

    // re-throw excpetion
    if ( except) rethrow_exception( except);
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

    ptr_t tmp;
    std::swap( tmp, impl_);
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
