
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_TIMED_MUTEX_H
#define BOOST_FIBERS_TIMED_MUTEX_H

#include <chrono>

#include <boost/config.hpp>

#include <boost/fiber/context.hpp>
#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/clock_cast.hpp>
#include <boost/fiber/detail/queues.hpp>
#include <boost/fiber/detail/spinlock.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

class BOOST_FIBERS_DECL timed_mutex {
private:
    enum class mutex_status {
        locked = 0,
        unlocked
    };

    typedef detail::wait_queue< context >   wait_queue_t;

    detail::spinlock    splk_;
    mutex_status        state_;
    context::id         owner_;
    wait_queue_t            wait_queue_;

    bool lock_if_unlocked_();

    bool try_lock_until_( std::chrono::steady_clock::time_point const& timeout_time);

public:
    timed_mutex();

    ~timed_mutex();

    timed_mutex( timed_mutex const&) = delete;
    timed_mutex & operator=( timed_mutex const&) = delete;

    void lock();

    bool try_lock();

    template< typename Clock, typename Duration >
    bool try_lock_until( std::chrono::time_point< Clock, Duration > const& timeout_time_) {
        std::chrono::steady_clock::time_point timeout_time(
                detail::clock_cast( timeout_time_) );
        return try_lock_until_( timeout_time);
    }

    template< typename Rep, typename Period >
    bool try_lock_for( std::chrono::duration< Rep, Period > const& timeout_duration) {
        return try_lock_until_( std::chrono::steady_clock::now() + timeout_duration);
    }

    void unlock();
};

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_TIMED_MUTEX_H
