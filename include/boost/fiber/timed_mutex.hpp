
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_TIMED_MUTEX_H
#define BOOST_FIBERS_TIMED_MUTEX_H

#include <deque>

#include <boost/chrono/system_clocks.hpp>
#include <boost/config.hpp>
#include <boost/utility.hpp>

#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/worker_fiber.hpp>
#include <boost/fiber/detail/fiber_base.hpp>
#include <boost/fiber/detail/spinlock.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

# if defined(BOOST_MSVC)
# pragma warning(push)
# pragma warning(disable:4355 4251 4275)
# endif

namespace boost {
namespace fibers {

class BOOST_FIBERS_DECL timed_mutex : private noncopyable
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
    std::deque< detail::fiber_base * >  waiting_;

    bool lock_if_unlocked_();

public:
    timed_mutex();

    ~timed_mutex();

    void lock();

    bool try_lock();

    bool try_lock_until( chrono::high_resolution_clock::time_point const& timeout_time);

    template< typename ClockType >
    bool try_lock_until( typename ClockType::time_point const& timeout_time_)
    {
        chrono::high_resolution_clock::time_point timeout_time(
                detail::convert_tp( timeout_time_) );
        return try_lock_until( timeout_time);
    }

    template< typename Rep, typename Period >
    bool try_lock_for( chrono::duration< Rep, Period > const& timeout_duration)
    { return try_lock_until( chrono::high_resolution_clock::now() + timeout_duration); }

    void unlock();
};

}}

# if defined(BOOST_MSVC)
# pragma warning(pop)
# endif

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_TIMED_MUTEX_H
