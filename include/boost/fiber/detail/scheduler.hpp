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

#if ! (defined(__APPLE__) && defined(BOOST_HAS_PTHREADS))

#   if defined(_MSC_VER) || defined(__BORLANDC__) || defined(__DMC__) || \
        (defined(__ICC) && defined(BOOST_WINDOWS))
#       define TLS_VAR_DECL(type) __declspec(thread) type
#   else
#       define TLS_VAR_DECL(type) __thread type
#   endif // if defined(_MSC_VER) || defined(__BORLANDC__) || ...

// generic thread_local_ptr
template< typename T >
class thread_local_ptr : private noncopyable
{
private:
    typedef void ( * cleanup_function)( T*);

    static TLS_VAR_DECL(T) *t_;
    cleanup_function cf_;

public:
    thread_local_ptr() BOOST_NOEXCEPT :
        cf_(0)
    {}

    thread_local_ptr( cleanup_function cf) BOOST_NOEXCEPT :
        cf_( cf)
    {}

    BOOST_EXPLICIT_OPERATOR_BOOL();

    T * get() const BOOST_NOEXCEPT
    { return t_; }

    bool operator!() const BOOST_NOEXCEPT
    { return ! get(); }

    bool operator==( thread_local_ptr const& other) BOOST_NOEXCEPT
    { return this->get() == other.get(); }

    bool operator!=( thread_local_ptr const& other) BOOST_NOEXCEPT
    { return ! ( * this == other); }

    void reset( T * t) BOOST_NOEXCEPT
    { t_ = t; }
};

template< typename T >
TLS_VAR_DECL(T) * thread_local_ptr< T >::t_ = 0;

#undef TLS_VAR_DECL

#else  // Mac-specific thread_local_ptr
template< typename T >
class thread_local_ptr : private noncopyable
{
private:
    typedef void ( * cleanup_function)( T*);

    ::pthread_key_t     key_;

    static void deleter_fn( void * ptr )
    {
        T* obj = static_cast<T*>(ptr);
        delete obj;
    }

public:
    /// By default, use a thread-exit cleanup function that deletes T*.
    thread_local_ptr() BOOST_NOEXCEPT
    {
        int ok = ::pthread_key_create( & key_, &thread_local_ptr::deleter_fn );
        BOOST_ASSERT( ok == 0 );
        (void)ok;
    }

    /// Allow caller to override cleanup function, 0 to suppress
    thread_local_ptr( cleanup_function cf ) BOOST_NOEXCEPT
    {
        int ok = ::pthread_key_create( & key_, cf );
        BOOST_ASSERT( ok == 0 );
        (void)ok;
    }

    BOOST_EXPLICIT_OPERATOR_BOOL();

    T * get() const BOOST_NOEXCEPT
    { return static_cast< T * >( ::pthread_getspecific( key_) ); }

    bool operator!() const BOOST_NOEXCEPT
    { return ! get(); }

    bool operator==( thread_local_ptr const& other) BOOST_NOEXCEPT
    { return this->get() == other.get(); }

    bool operator!=( thread_local_ptr const& other) BOOST_NOEXCEPT
    { return ! ( * this == other); }

    void reset( T * ptr) BOOST_NOEXCEPT
    { ::pthread_setspecific( key_, ptr); }
};
#endif  // Mac-specific thread_local_ptr

class scheduler : private noncopyable
{
private:
    static thread_local_ptr< fiber_manager > instance_;

public:
    template< typename F >
    static fiber_base * extract( F const& f) BOOST_NOEXCEPT
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
