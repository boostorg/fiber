
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_DETAIL_AUTORESET_EVENT_H
#define BOOST_FIBERS_DETAIL_AUTORESET_EVENT_H

#include <condition_variable>
#include <chrono>
#include <mutex>

#include <boost/config.hpp>

#include <boost/fiber/detail/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace detail {

class autoreset_event {
private:
    std::mutex              mtx_;
    std::condition_variable cnd_;
    bool                    flag_;

public:
    autoreset_event() :
        mtx_(), cnd_(), flag_( false) {
    }

    autoreset_event( autoreset_event const&) = delete;
    autoreset_event & operator=( autoreset_event const&) = delete;

    void set() {
        std::unique_lock< std::mutex > lk( mtx_);
        flag_ = true;
        lk.unlock();
        cnd_.notify_all();
    }

    void reset( std::chrono::steady_clock::time_point const& time_point) {
        std::unique_lock< std::mutex > lk( mtx_);
        if ( cnd_.wait_until( lk, time_point, [=](){ return flag_; }) ) {
            flag_ = false;
        }
    }
};

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DETAIL_AUTORESET_EVENT_H
