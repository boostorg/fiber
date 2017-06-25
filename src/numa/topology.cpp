
//          Copyright Oliver Kowalke 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/fiber/numa/topology.hpp"

#include <stdexcept>

#ifdef BOOST_HAS_ABI_HEADERS
# include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibes {
namespace numa {

#if BOOST_COMP_CLANG || \
    BOOST_COMP_GNUC || \
    BOOST_COMP_INTEL ||  \
    BOOST_COMP_MSVC 
# pragma message "topology() not supported"
#endif

BOOST_FIBERS_DECL
std::vector< topology > topology() {
    throw std::runtime_error("topology() not supported");
}

}}}

#ifdef BOOST_HAS_ABI_HEADERS
# include BOOST_ABI_SUFFIX
#endif
