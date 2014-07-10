
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_DETAIL_CONVERT_H
#define BOOST_FIBERS_DETAIL_CONVERT_H

#include <boost/chrono/system_clocks.hpp>
#include <boost/config.hpp>

#include <boost/fiber/detail/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace detail {

inline
chrono::high_resolution_clock::time_point convert_tp( chrono::high_resolution_clock::time_point const& timeout_time)
{ return timeout_time; }

template< typename TimePointType >
chrono::high_resolution_clock::time_point convert_tp( TimePointType const& timeout_time)
{
    typedef typename TimePointType::clock ClockType;
    return chrono::high_resolution_clock::now() + ( timeout_time - ClockType::now() );
}

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DETAIL_CONVERT_H
