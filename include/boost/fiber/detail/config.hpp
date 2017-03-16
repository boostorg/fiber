
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_DETAIL_CONFIG_H
#define BOOST_FIBERS_DETAIL_CONFIG_H

#include <cstddef>

#include <boost/config.hpp>
#include <boost/predef.h> 
#include <boost/detail/workaround.hpp>

#ifdef BOOST_FIBERS_DECL
# undef BOOST_FIBERS_DECL
#endif

#if (defined(BOOST_ALL_DYN_LINK) || defined(BOOST_FIBERS_DYN_LINK) ) && ! defined(BOOST_FIBERS_STATIC_LINK)
# if defined(BOOST_FIBERS_SOURCE)
#  define BOOST_FIBERS_DECL BOOST_SYMBOL_EXPORT
#  define BOOST_FIBERS_BUILD_DLL
# else
#  define BOOST_FIBERS_DECL BOOST_SYMBOL_IMPORT
# endif
#endif

#if ! defined(BOOST_FIBERS_DECL)
# define BOOST_FIBERS_DECL
#endif

#if ! defined(BOOST_FIBERS_SOURCE) && ! defined(BOOST_ALL_NO_LIB) && ! defined(BOOST_FIBERS_NO_LIB)
# define BOOST_LIB_NAME boost_fiber
# if defined(BOOST_ALL_DYN_LINK) || defined(BOOST_FIBERS_DYN_LINK)
#  define BOOST_DYN_LINK
# endif
# include <boost/config/auto_link.hpp>
#endif

#if BOOST_OS_LINUX || BOOST_OS_WINDOWS
# define BOOST_FIBERS_HAS_FUTEX
#endif

#if (!defined(BOOST_FIBERS_HAS_FUTEX) && \
    (defined(BOOST_FIBERS_SPINLOCK_TTAS_FUTEX) || defined(BOOST_FIBERS_SPINLOCK_TTAS_ADAPTIVE_FUTEX)))
# error "futex not supported on this platform"
#endif

#if !defined(BOOST_FIBERS_SPIN_MAX_COLLISIONS)
# define BOOST_FIBERS_SPIN_MAX_COLLISIONS 16
#endif

#if !defined(BOOST_FIBERS_SPIN_MAX_TESTS)
# define BOOST_FIBERS_SPIN_MAX_TESTS 100
#endif

// modern architectures have cachelines with 64byte length
// ARM Cortex-A15 32/64byte, Cortex-A9 16/32/64bytes
// MIPS 74K: 32byte, 4KEc: 16byte
// ist shoudl be safe to use 64byte for all
# define CACHE_ALIGNMENT 64
# define CACHELINE_LENGTH 64

#if defined(BOOST_NO_CXX11_ALIGNAS)
#  define cache_alignment CACHE_ALIGNMENT
#  define cacheline_length CACHELINE_LENGTH
#  define BOOST_FIBER_ALIGNAS_BEGIN(alignment)
#  define BOOST_FIBER_ALIGNAS_END(alignment) __attribute__((aligned (alignment)))
#else
   static constexpr std::size_t cache_alignment{ CACHE_ALIGNMENT };
   static constexpr std::size_t cacheline_length{ CACHELINE_LENGTH };
#  define BOOST_FIBER_ALIGNAS_BEGIN(alignment) alignas(alignment)
#  define BOOST_FIBER_ALIGNAS_END(alignment)
#endif
#define BOOST_FIBER_ALIGNAS(alignment, subject) BOOST_FIBER_ALIGNAS_BEGIN(alignment) subject BOOST_FIBER_ALIGNAS_END(alignment)

#if defined(BOOST_NO_CXX11_THREAD_LOCAL)
#  include <boost/thread/tss.hpp>
namespace boost {
namespace fibers {
  template<typename T>
  class initialized_thread_specific_ptr {
    public:
    initialized_thread_specific_ptr() {
      tss_.reset(new T);
    }

    T& operator*() {
      return *tss_;
    }

    private:
      boost::thread_specific_ptr<T> tss_;
  };
}}
#  define BOOST_FIBER_DECLARE_THREAD_LOCAL(type, name) boost::fibers::initialized_thread_specific_ptr<type> name
#  define BOOST_FIBER_DEFINE_THREAD_LOCAL(type, name) boost::fibers::initialized_thread_specific_ptr<type> name
#  define BOOST_FIBER_USE_THREAD_LOCAL(val) (*val)
#else
#  define BOOST_FIBER_DECLARE_THREAD_LOCAL(type, name) thread_local type name
#  define BOOST_FIBER_DEFINE_THREAD_LOCAL(type, name) thread_local type name
#  define BOOST_FIBER_USE_THREAD_LOCAL(val) val
#endif

#endif // BOOST_FIBERS_DETAIL_CONFIG_H
