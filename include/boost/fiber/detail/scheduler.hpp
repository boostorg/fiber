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

template< typename T >
class thread_local_ptr : private noncopyable
{
private:
    typedef void ( * cleanup_function)( T*);

#if defined(_MSC_VER) || defined(__BORLANDC__) || defined(__DMC__) || \
    (defined(__ICC) && defined(BOOST_WINDOWS))
    static volatile __declspec(thread) T         *   t_;
#elif defined(__APPLE__) && defined(BOOST_HAS_PTHREADS)
    static volatile detail::thread_local_ptr< T >    t_;
#else
    static volatile __thread T                   *   t_;
#endif
    cleanup_function                                 cf_;

public:
    thread_local_ptr( cleanup_function cf) BOOST_NOEXCEPT :
        cf_( cf)
    {}

    BOOST_EXPLICIT_OPERATOR_BOOL();

    volatile T * get() const BOOST_NOEXCEPT volatile
    { return t_; }

    bool operator!() const BOOST_NOEXCEPT
    { return ! get(); }

    bool operator==( thread_local_ptr const& other) BOOST_NOEXCEPT
    { return this->get() == other.get(); }

    bool operator!=( thread_local_ptr const& other) BOOST_NOEXCEPT
    { return ! ( * this == other); }

    void reset( T * t) BOOST_NOEXCEPT volatile
    { t_ = t; }
};

class scheduler : private noncopyable
{
private:
    static volatile thread_local_ptr< fiber_manager > instance_;

public:
    template< typename F >
    static worker_fiber * extract( F const& f) BOOST_NOEXCEPT
    { return f.impl_; }

    static volatile fiber_manager * instance()
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
