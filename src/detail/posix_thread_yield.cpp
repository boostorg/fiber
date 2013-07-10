
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/fiber/detail/thread_yield.hpp"

#include <boost/assert.hpp>
#include <boost/config.hpp>

#if defined(BOOST_HAS_SCHED_YIELD)
#include <sched.h>
#elif defined(BOOST_HAS_NONOSLEEP)
#include <time.h>
#endif

namespace boost {
namespace fibers {
namespace detail {

void thread_yield()
{
#if defined(BOOST_HAS_SCHED_YIELD)
    BOOST_ASSERT( 0 == ::sched_yield() );
#elif defined(BOOST_HAS_NONOSLEEP)
    timespec ts = { 0, 0 };
    ::nanosleep( & ts, 0);
#else
# warning "no implementation for thread_yield()"
#endif
}

}}}
