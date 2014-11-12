//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/assert.hpp>
#include "boost/fiber/properties.hpp"
#include "boost/fiber/algorithm.hpp"
#include "boost/fiber/fiber_manager.hpp"

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

void fiber_properties::notify()
{
    BOOST_ASSERT(sched_algo_);
    sched_algo_->property_change(fiber_, this);
}

}}                                  // boost::fiber

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
