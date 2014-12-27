
//          Copyright Oliver Kowalke 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_PROTECTED_FIXEDSIZE_H
#define BOOST_FIBERS_PROTECTED_FIXEDSIZE_H

#include <boost/config.hpp>
#include <boost/context/protected_fixedsize.hpp>

#include <boost/fiber/detail/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

typedef boost::context::protected_fixedsize protected_fixedsize;

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_PROTECTED_FIXEDSIZE_H
