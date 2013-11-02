//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_QUEUE_OP_STATUS_H
#define BOOST_FIBERS_QUEUE_OP_STATUS_H

#include <boost/config.hpp>
#include <boost/detail/scoped_enum_emulation.hpp>

#include <boost/fiber/detail/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

BOOST_SCOPED_ENUM_DECLARE_BEGIN(queue_op_status)
{
    success = 0,
    empty,
    full,
    closed,
    timeout
}
BOOST_SCOPED_ENUM_DECLARE_END(queue_op_status)

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_QUEUE_OP_STATUS_H
