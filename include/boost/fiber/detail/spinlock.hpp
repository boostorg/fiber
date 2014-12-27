
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
//  based on boost::interprocess::sync::interprocess_spin::mutex

#ifndef BOOST_FIBERS_SPINLOCK_H
#define BOOST_FIBERS_SPINLOCK_H

#include <atomic>

#include <boost/fiber/detail/config.hpp>

namespace boost {
namespace fibers {
namespace detail {

class BOOST_FIBERS_DECL spinlock {
private:
    enum class spinlock_status {
        locked = 0,
        unlocked
    };

    std::atomic< spinlock_status >  state_;

public:
    spinlock() noexcept;

    spinlock( spinlock const&) = delete;
    spinlock & operator=( spinlock const&) = delete;

    void lock();

    void unlock() noexcept;
};

}}}

#endif // BOOST_FIBERS_SPINLOCK_H
