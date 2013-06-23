//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_THIS_FIBER_OPERATIONS_H
#define BOOST_THIS_FIBER_OPERATIONS_H

#include <boost/fiber/algorithm.hpp>
#include <boost/fiber/detail/scheduler.hpp>
#include <boost/fiber/fiber.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace this_fiber {

inline
fibers::fiber::id get_id()
{
	return fibers::detail::scheduler::instance()->active()
	    ? fibers::detail::scheduler::instance()->active()->get_id()
        : fibers::fiber::id();
}

inline
void yield()
{ fibers::detail::scheduler::instance()->yield(); }

}

namespace fibers {

inline
algorithm * scheduling_algorithm( algorithm * al)
{ return detail::scheduler::replace( al); }

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_THIS_FIBER_OPERATIONS_H
