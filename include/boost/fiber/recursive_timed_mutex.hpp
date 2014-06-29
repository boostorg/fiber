
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
//  based on boost::interprocess::sync::interprocess_spinlock

#ifndef BOOST_FIBERS_RECURSIVE_TIMED_MUTEX_H
#define BOOST_FIBERS_RECURSIVE_TIMED_MUTEX_H

#include <cstddef>
#include <deque>

#include <boost/config.hpp>
#include <boost/utility.hpp>

#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/worker_fiber.hpp>
#include <boost/fiber/detail/spinlock.hpp>
#include <boost/fiber/detail/fiber_base.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

# if defined(BOOST_MSVC)
# pragma warning(push)
# pragma warning(disable:4355 4251 4275)
# endif

namespace boost {
namespace fibers {

class BOOST_FIBERS_DECL recursive_timed_mutex : private noncopyable
{
private:
    enum state_t
    {
        LOCKED = 0,
        UNLOCKED
    };

    detail::spinlock                    splk_;
    state_t                             state_;
    detail::worker_fiber::id            owner_;
    std::size_t                         count_;
    std::deque< detail::fiber_base * >  waiting_;

public:
    recursive_timed_mutex();

    ~recursive_timed_mutex();

    void lock();

    bool try_lock();

    bool try_lock_until( clock_type::time_point const& timeout_time);

    template< typename Rep, typename Period >
    bool try_lock_for( chrono::duration< Rep, Period > const& timeout_duration)
    { return try_lock_until( clock_type::now() + timeout_duration); }

    void unlock();
};

}}

# if defined(BOOST_MSVC)
# pragma warning(pop)
# endif

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_RECURSIVE_TIMED_MUTEX_H
