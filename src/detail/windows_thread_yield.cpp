
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_FIBERS_SOURCE

#include <boost/fiber/detail/thread_yield.hpp>

#include <windows.h>

namespace boost {
namespace fibers {
namespace detail {

void thread_yield()
{
    ::Sleep( 0);
}

}}}
