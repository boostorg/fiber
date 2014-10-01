//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_DETAIL_SCHEDULER_H
#define BOOST_FIBERS_DETAIL_SCHEDULER_H

#if defined(__APPLE__) && defined(BOOST_HAS_PTHREADS)
#include <pthread.h>  // pthread_key_create, pthread_[gs]etspecific
#endif

#include <boost/asio.hpp>
#include <boost/config.hpp>
#include <boost/utility.hpp>
#include <boost/utility/explicit_operator_bool.hpp>

#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/fiber_base.hpp>
#include <boost/fiber/fiber_manager.hpp>
#include <boost/fiber/thread_local_ptr.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

# if defined(BOOST_MSVC)
# pragma warning(push)
# pragma warning(disable:4251 4275)
# endif

namespace boost {
namespace fibers {
namespace detail {

class scheduler : private noncopyable
{
private:
    static thread_local_ptr< fiber_manager > instance_;
    //static thread_specific_ptr< fiber_manager > instance_;

public:
    template< typename F >
    static worker_fiber * extract( F const& f) BOOST_NOEXCEPT
    { return f.impl_.get(); }

    static fiber_manager * instance()
    {
        if ( ! instance_.get() )
            instance_.reset( new fiber_manager() );
        return instance_.get();
    }

    static void replace( sched_algorithm *);
};

}}}

# if defined(BOOST_MSVC)
# pragma warning(pop)
# endif

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DETAIL_SCHEDULER_H
