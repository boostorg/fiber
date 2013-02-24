
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_FIBERS_SOURCE

#include <boost/fiber/fiber.hpp>

#include <boost/assert.hpp>
#include <boost/exception/all.hpp>
#include <boost/scope_exit.hpp>
#include <boost/system/error_code.hpp>

#include <boost/fiber/detail/scheduler.hpp>
#include <boost/fiber/exceptions.hpp>
#include <boost/fiber/operations.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

void
fiber::start_fiber_()
{ detail::scheduler::instance().spawn( fiber_data_); }

int
fiber::priority() const
{
    BOOST_ASSERT( fiber_data_);

    return fiber_data_->priority();
}

void
fiber::priority( int prio)
{
    BOOST_ASSERT( fiber_data_);

    detail::scheduler::instance().priority( fiber_data_, prio);
}

void
fiber::join()
{
    BOOST_ASSERT( fiber_data_);

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

    try
    { detail::scheduler::instance().join( fiber_data_); }
    catch (...)
    {
        fiber_data_.reset();
        throw;
    }
    fiber_data_.reset();
}

void
fiber::interrupt()
{
    BOOST_ASSERT( fiber_data_);

    fiber_data_->request_interruption( true);
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
