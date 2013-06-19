//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_DETAIL_SCHEDULER_H
#define BOOST_FIBERS_DETAIL_SCHEDULER_H

#if defined(__APPLE__) && defined(BOOST_HAS_PTHREADS)
#include <pthread.h>                // pthread_key_create, pthread_[gs]etspecific
#endif

#include <boost/config.hpp>
#include <boost/utility.hpp>

#include <boost/fiber/algorithm.hpp>
#include <boost/fiber/detail/config.hpp>

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

// thread_local_ptr was contributed by Nat Goodspeed
#if defined(__APPLE__) && defined(BOOST_HAS_PTHREADS)
class thread_local_ptr : private noncopyable
{
private:
    struct dummy
    { void nonnull() {} };

    typedef void ( dummy::*safe_bool)();

    ::pthread_key_t     key_;

public:
    thread_local_ptr() BOOST_NOEXCEPT
    { BOOST_ASSERT( ! ::pthread_key_create( & key_, 0) ); }

    algorithm * get() const BOOST_NOEXCEPT
    { return static_cast< algorithm * >( ::pthread_getspecific( key_) ); }

    thread_local_ptr & operator=( algorithm * ptr) BOOST_NOEXCEPT
    {
        ::pthread_setspecific( key_, ptr);
        return * this;
    }

    algorithm & operator*() const BOOST_NOEXCEPT
    { return * get(); }

    algorithm * operator->() const BOOST_NOEXCEPT
    { return get(); }

    operator algorithm * () const BOOST_NOEXCEPT
    { return get(); }

    operator safe_bool() const BOOST_NOEXCEPT
    { return get() ? 0 : & dummy::nonnull; }

    bool operator!() const BOOST_NOEXCEPT
    { return ! get(); }

    bool operator==( thread_local_ptr const& other) BOOST_NOEXCEPT
    { return this->get() == other.get(); }

    bool operator!=( thread_local_ptr const& other) BOOST_NOEXCEPT
    { return ! ( * this == other); }
};

}
#endif

class BOOST_FIBERS_DECL scheduler : private noncopyable
{
private:
#if defined(_MSC_VER) || defined(__BORLANDC__) || defined(__DMC__) || \
    (defined(__ICC) && defined(BOOST_WINDOWS))
    static __declspec(thread) algorithm     *   instance_;
#elif defined(BOOST_MAC_PTHREADS)
    static detail::thread_local_ptr             instance_;
#else
    static __thread algorithm               *   instance_;
#endif

public:
    template< typename F >
    static fiber_base::ptr_t extract( F const& f) {
        return f.impl_;
    }

    static algorithm & instance() BOOST_NOEXCEPT;

    static algorithm * replace( algorithm *) BOOST_NOEXCEPT;
};

}}}

# if defined(BOOST_MSVC)
# pragma warning(pop)
# endif

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DETAIL_SCHEDULER_H
