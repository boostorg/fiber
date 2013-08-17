
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_FUTURE_STATUS_HPP
#define BOOST_FIBERS_FUTURE_STATUS_HPP

#include <boost/config.hpp>
#include <boost/detail/scoped_enum_emulation.hpp>

namespace boost {
namespace fibers {

BOOST_SCOPED_ENUM_DECLARE_BEGIN(future_status)
{
    ready = 1,
    timeout,
    deferred
}
BOOST_SCOPED_ENUM_DECLARE_END(future_status)

}}

#endif // BOOST_FIBERS_FUTURE_STATUS_HPP
