//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_FIBERS_SOURCE

#include <boost/fiber/detail/scheduler.hpp>

#include <boost/assert.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace detail {

#if defined(_MSC_VER) || defined(__BORLANDC__) || defined(__DMC__) || \
    (defined(__ICC) && defined(BOOST_WINDOWS))
__declspec(thread) algorithm * scheduler::instance_ = 0;
#elif defined(BOOST_MAC_PTHREADS)
detail::thread_local_ptr scheduler::instance_ = 0;
#else
//algorithm * scheduler::instance_ = 0;
__thread algorithm * scheduler::instance_ = 0;
#endif

algorithm &
scheduler::instance() BOOST_NOEXCEPT
{
    BOOST_ASSERT( instance_);
	return * instance_;
}

algorithm *
scheduler::replace( algorithm * other) BOOST_NOEXCEPT
{
    algorithm * old = instance_;
    instance_ = other;
    return old;
}

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
