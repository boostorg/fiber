//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_SCHEDULER_H
#define BOOST_FIBERS_SCHEDULER_H

#if defined(__APPLE__) && defined(BOOST_HAS_PTHREADS)
#include <pthread.h>                // pthread_key_create, pthread_[gs]etspecific
#endif

#include <boost/assert.hpp>
#include <boost/chrono/system_clocks.hpp>
#include <boost/config.hpp>
#include <boost/utility.hpp>

#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/fiber.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

# if defined(BOOST_MSVC)
# pragma warning(push)
# pragma warning(disable:4251 4275)
# endif

namespace boost {
namespace fibers {

// thread_local_ptr was a contribution from
// Nat Goodspeed
#if defined(__APPLE__) && defined(BOOST_HAS_PTHREADS)
class scheduler;

namespace detail {

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

    scheduler * get() const BOOST_NOEXCEPT
    { return static_cast< scheduler * >( ::pthread_getspecific( key_) ); }

    thread_local_ptr & operator=( scheduler * ptr) BOOST_NOEXCEPT
    {
        ::pthread_setspecific( key_, ptr);
        return * this;
    }

    scheduler & operator*() const BOOST_NOEXCEPT
    { return * get(); }

    scheduler * operator->() const BOOST_NOEXCEPT
    { return get(); }

    operator scheduler * () const BOOST_NOEXCEPT
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
    static __declspec(thread) scheduler     *   instance_;
#elif defined(BOOST_MAC_PTHREADS)
    static detail::thread_local_ptr             instance_;
#else
    static __thread scheduler               *   instance_;
#endif

public:
    static scheduler & instance();

    static void swap( scheduler *) BOOST_NOEXCEPT;

    virtual void spawn( detail::fiber_base::ptr_t const&) = 0;

    virtual void join( detail::fiber_base::ptr_t const&) = 0;

    virtual void cancel( detail::fiber_base::ptr_t const&) = 0;

    virtual void notify( detail::fiber_base::ptr_t const&) = 0;

    virtual detail::fiber_base::ptr_t active() = 0;

    virtual void sleep( chrono::system_clock::time_point const& abs_time) = 0;

    virtual bool run() = 0;

    virtual void wait() = 0;

    virtual void yield() = 0;

    virtual ~scheduler() {}
};

}}

# if defined(BOOST_MSVC)
# pragma warning(pop)
# endif

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_SCHEDULER_H
