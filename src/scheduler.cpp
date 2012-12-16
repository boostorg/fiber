//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_FIBERS_SOURCE

#include <boost/fiber/scheduler.hpp>

#include <boost/fiber/detail/default_scheduler.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

#if defined(_MSC_VER) || defined(__BORLANDC__) || defined(__DMC__) || \
    (defined(__ICC) && defined(BOOST_WINDOWS))
__declspec(thread) scheduler * scheduler::instance_ = 0;
#elif defined(BOOST_MAC_PTHREADS)
detail::thread_local_ptr scheduler::instance_ = 0;
#else
__thread scheduler * scheduler::instance_ = 0;
#endif

scheduler &
scheduler::instance()
{
    if ( ! instance_) instance_ = new detail::default_scheduler();
	return * instance_;
}

scheduler *
scheduler::replace( scheduler * other)
{
    scheduler * old = instance_;
    instance_ = other;
    return old;
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
