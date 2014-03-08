//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/fiber/detail/scheduler.hpp"

#include <boost/assert.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace detail {

static void deleter_fn( algorithm * algo) { delete algo; }
static void null_deleter_fn( algorithm *) {}

#if defined(_MSC_VER) || defined(__BORLANDC__) || defined(__DMC__) || \
    (defined(__ICC) && defined(BOOST_WINDOWS))
template< typename T >
__declspec(thread) T * thread_local_ptr< T >::t_ = 0;
#elif defined(__APPLE__) && defined(BOOST_HAS_PTHREADS)
template< typename T >
detail::thread_local_ptr< T > thread_local_ptr< T >::t_;
#else
template< typename T >
__thread T * thread_local_ptr< T >::t_ = 0;
#endif

thread_local_ptr< algorithm > scheduler::default_algo_( deleter_fn);
thread_local_ptr< algorithm > scheduler::instance_( null_deleter_fn);

void
scheduler::replace( algorithm * other)
{
    BOOST_ASSERT( other);

    instance_.reset( other);
}

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
