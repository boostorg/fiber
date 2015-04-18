
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
//  based on boost::interprocess::sync::interprocess_spinlock

#ifndef BOOST_FIBERS_RECURSIVE_TIMED_MUTEX_H
#define BOOST_FIBERS_RECURSIVE_TIMED_MUTEX_H

#include <chrono>
#include <cstddef>
#include <deque>

#include <boost/config.hpp>

#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/convert.hpp>
#include <boost/fiber/detail/spinlock.hpp>
#include <boost/fiber/fiber_context.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

class BOOST_FIBERS_DECL recursive_timed_mutex
{
private:
    enum class mutex_status {
        locked = 0,
        unlocked
    };

    detail::spinlock                splk_;
    mutex_status                    state_;
    fiber_context::id               owner_;
    std::size_t                     count_;
    std::deque< fiber_context * >   waiting_;

    bool lock_if_unlocked_();

public:
    recursive_timed_mutex();

    ~recursive_timed_mutex();

    recursive_timed_mutex( recursive_timed_mutex const&) = delete;
    recursive_timed_mutex & operator=( recursive_timed_mutex const&) = delete;

    void lock();

    bool try_lock();

    bool try_lock_until( std::chrono::high_resolution_clock::time_point const& timeout_time);

    template< typename Clock, typename Duration >
    bool try_lock_until( std::chrono::time_point< Clock, Duration > const& timeout_time_) {
        std::chrono::high_resolution_clock::time_point timeout_time(
                detail::convert_tp( timeout_time_) );
        return try_lock_until( timeout_time);
    }

    template< typename Rep, typename Period >
    bool try_lock_for( std::chrono::duration< Rep, Period > const& timeout_duration) {
        return try_lock_until( std::chrono::high_resolution_clock::now() + timeout_duration);
    }

    void unlock();
};

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_RECURSIVE_TIMED_MUTEX_H
