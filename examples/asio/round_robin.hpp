//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_ASIO_ROUND_ROBIN_H
#define BOOST_FIBERS_ASIO_ROUND_ROBIN_H

#include <chrono>

#include <boost/asio.hpp>
#include <boost/assert.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/config.hpp>

#include <boost/fiber/context.hpp>
#include <boost/fiber/operations.hpp>
#include <boost/fiber/scheduler.hpp>
#include "yield.hpp"

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace asio {

class round_robin : public boost::fibers::sched_algorithm {
private:
    boost::asio::io_service             &   io_svc_;
    boost::asio::steady_timer               suspend_timer_;
    boost::asio::steady_timer               keepalive_timer_;
    std::chrono::steady_clock::duration     keepalive_interval_;
    boost::fibers::scheduler::ready_queue_t ready_queue_;

public:
    round_robin( boost::asio::io_service & io_svc,
                 std::chrono::steady_clock::duration keepalive_interval = std::chrono::milliseconds(250) ) :
        io_svc_( io_svc),
        suspend_timer_( io_svc_),
        keepalive_timer_( io_svc_),
        keepalive_interval_( keepalive_interval),
        ready_queue_() {
        on_empty_io_service();
    }

    void awakened( boost::fibers::context * ctx) {
        BOOST_ASSERT( nullptr != ctx);
        BOOST_ASSERT( ! ctx->ready_is_linked() );
        ctx->ready_link( ready_queue_);
    }

    boost::fibers::context * pick_next() {
        boost::fibers::context * ctx( nullptr);
        if ( ! ready_queue_.empty() ) {
            ctx = & ready_queue_.front();
            ready_queue_.pop_front();
            BOOST_ASSERT( nullptr != ctx);
            BOOST_ASSERT( ! ctx->ready_is_linked() );
        }
        return ctx;
    }

    bool has_ready_fibers() const noexcept {
        return ! ready_queue_.empty();
    }

    void suspend_until( std::chrono::steady_clock::time_point const& suspend_time) {
        suspend_timer_.expires_at( suspend_time);
        boost::system::error_code ignored_ec;
        suspend_timer_.async_wait( boost::fibers::asio::yield[ignored_ec]);
    }

    void notify() {
        suspend_timer_.expires_at( std::chrono::steady_clock::now() );
    }

    void on_empty_io_service() {
        io_svc_.post([](){ boost::this_fiber::yield(); });
        keepalive_timer_.expires_from_now( keepalive_interval_);
        boost::system::error_code ignored_ec;
        keepalive_timer_.async_wait( std::bind( & round_robin::on_empty_io_service, this) );
    }
};

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_ASIO_ROUND_ROBIN_H
