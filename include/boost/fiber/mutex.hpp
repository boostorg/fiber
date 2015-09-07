
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_MUTEX_H
#define BOOST_FIBERS_MUTEX_H

#include <deque>

#include <boost/config.hpp>

#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/spinlock.hpp>
#include <boost/fiber/context.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

class BOOST_FIBERS_DECL mutex {
private:
    enum class mutex_status {
        locked = 0,
        unlocked
    };

    detail::spinlock                splk_;
    mutex_status                    state_;
    context::id               owner_;
    std::deque< context * >   waiting_;

    bool lock_if_unlocked_();

public:
    mutex();

    ~mutex();

    mutex( mutex const&) = delete;
    mutex & operator=( mutex const&) = delete;

    void lock();

    bool try_lock();

    void unlock();
};

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_MUTEX_H
