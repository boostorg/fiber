
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_DETAIL_INTERRUPT_FLAGS_H
#define BOOST_FIBERS_DETAIL_INTERRUPT_FLAGS_H

#include <boost/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace detail {

enum interrupt_t {
	INTERRUPTION_DISABLED = 1 << 1,
	INTERRUPTION_ENABLED  = 1 << 2,
	INTERRUPTION_BLOCKED  = 1 << 3
};

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DETAIL_INTERRUPT_FLAGS_H
