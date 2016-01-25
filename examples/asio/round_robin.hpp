//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_ASIO_ROUND_ROBIN_H
#define BOOST_FIBERS_ASIO_ROUND_ROBIN_H

#include <chrono>
#include <cstddef>

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

class round_robin : public boost::fibers::sched_algorithm,
                    public boost::asio::io_service::service {
private:
    std::size_t                             worker_counter_{ 0 };
    boost::asio::io_service             &   io_svc_;
    boost::asio::steady_timer               suspend_timer_;
    boost::fibers::scheduler::ready_queue_t ready_queue_{};

public:
    static boost::asio::io_service::id id;

    round_robin( boost::asio::io_service & io_svc) :
        boost::asio::io_service::service( io_svc),
        io_svc_( io_svc),
        suspend_timer_( io_svc_) {
        boost::asio::add_service( io_svc, this);
    }

    void awakened( boost::fibers::context * ctx) noexcept {
        BOOST_ASSERT( nullptr != ctx);
        BOOST_ASSERT( ! ctx->ready_is_linked() );
        ctx->ready_link( ready_queue_);
        if ( ! ctx->is_dispatcher_context() ) {
            ++worker_counter_;
        }
    }

    boost::fibers::context * pick_next() noexcept {
        boost::fibers::context * ctx( nullptr);
        if ( ! ready_queue_.empty() ) {
            ctx = & ready_queue_.front();
            ready_queue_.pop_front();
            BOOST_ASSERT( nullptr != ctx);
            BOOST_ASSERT( ! ctx->ready_is_linked() );
            if ( ! ctx->is_dispatcher_context() ) {
                BOOST_ASSERT( 0 < worker_counter_);
                --worker_counter_;
            }
        }
        return ctx;
    }

    bool has_ready_fibers() const noexcept {
        return 0 < worker_counter_;
    }

    void suspend_until( std::chrono::steady_clock::time_point const& suspend_time) noexcept {
        fprintf(stderr,"round_robin::suspend_until()\n");
        suspend_timer_.expires_at( suspend_time);
        boost::system::error_code ignored_ec;
        suspend_timer_.async_wait( boost::fibers::asio::yield[ignored_ec]);
    }

    void notify() noexcept {
        suspend_timer_.expires_at( std::chrono::steady_clock::now() );
    }

    void poll() {
        io_svc_.post([](){ boost::this_fiber::yield(); });
    }

    void shutdown_service() {
    }
};

boost::asio::io_service::id round_robin::id;

void run( boost::asio::io_service & io_svc) {
    BOOST_ASSERT( boost::asio::has_service< round_robin >( io_svc) );
    while ( ! io_svc.stopped() ) {
        if ( ! boost::asio::use_service< round_robin >( io_svc).has_ready_fibers() ) {
            fprintf(stderr,"before io_svc.run_one()\n");
            if ( ! io_svc.run_one() ) {
                fprintf(stderr,"after io_svc.run_one() == false\n");
                break;
            }
            fprintf(stderr,"after io_svc.run_one() == true\n");
        } else {
            while ( io_svc.poll() ) {
            }
            boost::asio::use_service< round_robin >( io_svc).poll();
        }
    }
}

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_ASIO_ROUND_ROBIN_H
