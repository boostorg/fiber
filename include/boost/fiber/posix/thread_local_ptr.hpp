
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_THREAD_LOCAL_PTR_H
#define BOOST_FIBERS_THREAD_LOCAL_PTR_H

#include <pthread.h>  // pthread_key_create, pthread_[gs]etspecific

#include <boost/config.hpp>
#include <boost/utility.hpp>
#include <boost/utility/explicit_operator_bool.hpp>

#include <boost/fiber/detail/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

# if ! (defined(__APPLE__) && defined(BOOST_HAS_PTHREADS))
template< typename T >
class thread_local_ptr : private noncopyable
{
private:
    static void cleanup_function( void * vp)
    {
        thread_local_ptr< T > * p = static_cast< thread_local_ptr< T > * >( vp);
        BOOST_ASSERT( 0 != p);
        delete p->t_;
        p->t_ = 0;
    }

    static __thread T       *   t_;

    pthread_key_t               key_;

public:
    thread_local_ptr() BOOST_NOEXCEPT :
        key_()
    { ::pthread_key_create( & key_, & thread_local_ptr::cleanup_function); }

    ~thread_local_ptr()
    { if ( 0 != t_) cleanup_function( t_); } // required for single-threaded apps

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
    {
        t_ = t;
        ::pthread_setspecific( key_, this);
    }
};
template< typename T >
__thread T * thread_local_ptr< T >::t_ = 0;
# else
template< typename T >
class thread_local_ptr : private noncopyable
{
private:
    static void cleanup_function( void * vp)
    {
        thread_local_ptr< T > * p = static_cast< thread_local_ptr< T > * >( vp);
        BOOST_ASSERT( 0 != p);
        T * t = static_cast< T * >( ::pthread_getspecific( p->t_key_) );
        delete t;
        ::pthread_setspecific( p->t_key_, 0);
    }

    pthread_key_t   this_key_;
    pthread_key_t   t_key_;

public:
    thread_local_ptr() BOOST_NOEXCEPT :
        this_key_(), t_key_()
    {
        ::pthread_key_create( & this_key_, & thread_local_ptr::cleanup_function);
        ::pthread_key_create( & t_key_, 0);
    }

    ~thread_local_ptr()
    { if ( 0 != t_) cleanup_function( t_); } // required for single-threaded apps

    BOOST_EXPLICIT_OPERATOR_BOOL();

    T * get() const BOOST_NOEXCEPT
    { return static_cast< T * >( ::pthread_getspecific( t_key_) ); }

    bool operator!() const BOOST_NOEXCEPT
    { return ! get(); }

    bool operator==( thread_local_ptr const& other) BOOST_NOEXCEPT
    { return this->get() == other.get(); }

    bool operator!=( thread_local_ptr const& other) BOOST_NOEXCEPT
    { return ! ( * this == other); }

    void reset( T * t) BOOST_NOEXCEPT
    {
        ::pthread_setspecific( this_key_, this);
        ::pthread_setspecific( t_key_, t);
    }
};
# endif

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_THREAD_LOCAL_PTR_H
