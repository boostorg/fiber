
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_DETAIL_FLAGS_H
#define BOOST_FIBERS_DETAIL_FLAGS_H

#include <boost/config.hpp>

#include <boost/fiber/detail/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace detail {

enum flag_t
{
    flag_forced_unwind  = 1 << 1,
    flag_unwind_stack   = 1 << 2,
    flag_preserve_fpu   = 1 << 3,
    flag_complete       = 1 << 4,
    flag_resumed        = 1 << 5,
    flag_canceled       = 1 << 6
};

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DETAIL_FLAGS_H
