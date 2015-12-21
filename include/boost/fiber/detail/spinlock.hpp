
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
//  based on boost::interprocess::sync::interprocess_spin::mutex

#ifndef BOOST_FIBERS_SPINLOCK_H
#define BOOST_FIBERS_SPINLOCK_H

#include <atomic>
#include <mutex>

#include <boost/fiber/detail/config.hpp>

namespace boost {
namespace fibers {
namespace detail {

class BOOST_FIBERS_DECL atomic_spinlock {
private:
    enum class spinlock_status {
        locked = 0,
        unlocked
    };

    std::atomic< spinlock_status >  state_{ spinlock_status::unlocked };

public:
    atomic_spinlock() noexcept = default;

    atomic_spinlock( atomic_spinlock const&) = delete;
    atomic_spinlock & operator=( atomic_spinlock const&) = delete;

    void lock() noexcept;
    void unlock() noexcept;
};

struct non_spinlock {
    constexpr non_spinlock() noexcept {}
    void lock() noexcept {}
    void unlock() noexcept {}
};

struct non_lock {
    constexpr non_lock( non_spinlock) noexcept {}
    void lock() noexcept {}
    void unlock() noexcept {}
};

#if ! defined(BOOST_FIBES_NO_ATOMICS) 
typedef atomic_spinlock spinlock;
using spinlock_lock = std::unique_lock< spinlock >;
#else
typedef non_spinlock    spinlock;
using spinlock_lock = non_lock;
#endif

}}}

#endif // BOOST_FIBERS_SPINLOCK_H
