//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_ALGORITHM_H
#define BOOST_FIBERS_ALGORITHM_H

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/thread/lock_types.hpp> 
#include <boost/utility.hpp>

#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/worker_fiber.hpp>
#include <boost/fiber/detail/fiber_base.hpp>
#include <boost/fiber/detail/spinlock.hpp>
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

struct algorithm : private noncopyable
{
    virtual void spawn( detail::worker_fiber *) = 0;

    virtual void priority( detail::worker_fiber *, int) BOOST_NOEXCEPT = 0;

    virtual void join( detail::worker_fiber *) = 0;

    virtual detail::worker_fiber * active() BOOST_NOEXCEPT = 0;

    virtual void run() = 0;

    virtual void wait( unique_lock< detail::spinlock > &) = 0;
    virtual bool wait_until( clock_type::time_point const&,
                             unique_lock< detail::spinlock > &) = 0;
    template< typename Rep, typename Period >
    bool wait_for( chrono::duration< Rep, Period > const& timeout_duration,
                   unique_lock< detail::spinlock > & lk)
    { return wait_until( clock_type::now() + timeout_duration, lk); }

    virtual void yield() = 0;

    virtual detail::fiber_base * get_main_fiber() = 0;

    virtual ~algorithm() {}
};

}}

# if defined(BOOST_MSVC)
# pragma warning(pop)
# endif

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_ALGORITHM_H
