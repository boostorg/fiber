
//          Copyright Oliver Kowalke / Nat Goodspeed 2015.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/fiber/algorithm.hpp"

#include <boost/config.hpp>

#include <boost/fiber/fiber_context.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

//static
fiber_properties *
sched_algorithm_with_properties_base::get_properties( fiber_context * f) noexcept {
    return f->get_properties();
}

//static
void
sched_algorithm_with_properties_base::set_properties( fiber_context * f, fiber_properties * props) noexcept {
    f->set_properties( props);
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
