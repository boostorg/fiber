//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/fiber/detail/scheduler.hpp"

#include <boost/assert.hpp>

#include "boost/fiber/asio/manager.hpp"

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace detail {

static void deleter_fn( fiber_manager * mgr) { delete mgr; }

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

thread_local_ptr< fiber_manager > scheduler::instance_( deleter_fn);

void
scheduler::replace( sched_algorithm * other)
{
    BOOST_ASSERT( other);

    instance()->set_sched_algo( other);
}

void
scheduler::register_io_svc( boost::asio::io_service & io_svc)
{ instance_.reset( new asio::manager( io_svc) ); }

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
