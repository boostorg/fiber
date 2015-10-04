//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef ASIO_SCHEDULER_H
#define ASIO_SCHEDULER_H

#include <chrono>

#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/config.hpp>

#include <boost/fiber/all.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

class asio_scheduler : public boost::fibers::sched_algorithm {
private:
    boost::asio::io_service             &   io_svc_;
    boost::asio::steady_timer               timer_;
    boost::fibers::scheduler::ready_queue_t ready_queue_;

    void do_yield() {
        boost::this_fiber::yield();
        io_svc_.post( std::bind( & asio_scheduler::do_yield, this) );
    }

public:
    asio_scheduler( boost::asio::io_service & io_svc) :
        io_svc_( io_svc),
        timer_( io_svc_),
        ready_queue_() {
        io_svc_.post( std::bind( & asio_scheduler::do_yield, this) );
    }

    void awakened( boost::fibers::context * ctx) {
        BOOST_ASSERT( nullptr != ctx);

        BOOST_ASSERT( ! ctx->ready_is_linked() );
        ctx->ready_link( ready_queue_);
    }

    boost::fibers::context * pick_next() {
        boost::fibers::context * victim( nullptr);
        if ( ! ready_queue_.empty() ) {
            victim = & ready_queue_.front();
            ready_queue_.pop_front();
            BOOST_ASSERT( nullptr != victim);
            BOOST_ASSERT( ! victim->ready_is_linked() );
        }
        return victim;
    }

    bool has_ready_fibers() const noexcept {
        return ! ready_queue_.empty();
    }

    void suspend_until( std::chrono::steady_clock::time_point const& suspend_time) {
        if ( std::chrono::steady_clock::now() < suspend_time) {
            timer_.expires_at( suspend_time);
            boost::system::error_code ignored_ec;
            timer_.wait( ignored_ec);
        }
    }

    void notify() {
        timer_.expires_at( std::chrono::steady_clock::now() );
    }
};

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // ASIO_SCHEDULER_H
