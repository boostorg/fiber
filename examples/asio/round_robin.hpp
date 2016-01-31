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

#include <boost/fiber/condition_variable.hpp>
#include <boost/fiber/context.hpp>
#include <boost/fiber/mutex.hpp>
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
    boost::asio::io_service                     &   io_svc_;
    boost::asio::steady_timer                       suspend_timer_;
    boost::fibers::scheduler::ready_queue_t         ready_queue_{};
    boost::fibers::mutex                            mtx_{};
    boost::fibers::condition_variable               cnd_{};
    std::size_t                                     counter_{ 0 };

public:
    struct service : public boost::asio::io_service::service {
        static boost::asio::io_service::id                  id;

        std::unique_ptr< boost::asio::io_service::work >    work_{};

        service( boost::asio::io_service & io_svc) :
            boost::asio::io_service::service( io_svc),
            work_{ new boost::asio::io_service::work( io_svc) } {
        }

        service( service const&) = delete;
        service & operator=( service const&) = delete;

        void shutdown_service() override final {
            work_.reset();
        }
    };

    round_robin( boost::asio::io_service & io_svc) :
        io_svc_( io_svc),
        suspend_timer_( io_svc_) {
        boost::asio::add_service< service >( io_svc_, new service( io_svc_) );
        io_svc_.post([this]() mutable {
                while ( ! io_svc_.stopped() ) {
                    if ( has_ready_fibers() ) {
                        // run all pending handlers in round_robin
                        while ( io_svc_.poll() );
                        // block this fiber till all pending (ready) fibers are processed
                        // == round_robin::suspend_until() has been called
                        std::unique_lock< boost::fibers::mutex > lk( mtx_);
                        cnd_.wait( lk);
                    } else {
                        // run one handler inside io_service
                        // if no handler available, block this thread
                        if ( ! io_svc_.run_one() ) {
                            break;
                        }
                    }
                }
            });
    }

    void awakened( boost::fibers::context * ctx) noexcept {
        BOOST_ASSERT( nullptr != ctx);
        BOOST_ASSERT( ! ctx->ready_is_linked() );
        ctx->ready_link( ready_queue_);
        if ( ! ctx->is_dispatcher_context() ) {
            ++counter_;
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
                --counter_;
            }
        }
        return ctx;
    }

    bool has_ready_fibers() const noexcept {
        return 0 < counter_;
    }

    void suspend_until( std::chrono::steady_clock::time_point const& abs_time) noexcept {
        if ( (std::chrono::steady_clock::time_point::max)() != abs_time) {
            suspend_timer_.expires_at( abs_time);
            suspend_timer_.async_wait([](boost::system::error_code const&){
                                        this_fiber::yield();
                                      });
        }
        cnd_.notify_one();
    }

    void notify() noexcept {
        suspend_timer_.expires_at( std::chrono::steady_clock::now() );
    }
};

boost::asio::io_service::id round_robin::service::id;

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_ASIO_ROUND_ROBIN_H
