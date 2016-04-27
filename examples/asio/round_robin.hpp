//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_ASIO_ROUND_ROBIN_H
#define BOOST_FIBERS_ASIO_ROUND_ROBIN_H

#include <chrono>
#include <cstddef>
#include <mutex>
#include <queue>

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

class round_robin : public sched_algorithm {
private:
    typedef scheduler::ready_queue_t rqueue_t;

    boost::asio::io_service                     &   io_svc_;
    boost::asio::steady_timer                       suspend_timer_;
    rqueue_t                                        rqueue_{};
    std::size_t                                     counter_{ 0 };

public:
    struct service : public boost::asio::io_service::service {
        static boost::asio::io_service::id                  id;

        std::unique_ptr< boost::asio::io_service::work >    work_;

        service( boost::asio::io_service & io_svc) :
            boost::asio::io_service::service( io_svc),
            work_{ new boost::asio::io_service::work( io_svc) } {
            io_svc.post([&io_svc](){
                            while ( ! io_svc.stopped() ) {
                                if ( boost::fibers::has_ready_fibers() ) {
                                    // run all pending handlers in round_robin
                                    while ( io_svc.poll() );
                                    // run pending (ready) fibers
                                    this_fiber::yield();
                                } else {
                                    // run one handler inside io_service
                                    // if no handler available, block this thread
                                    if ( ! io_svc.run_one() ) {
                                        break;
                                    }
                                }
                            }
                        });
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
        boost::asio::use_service< service >( io_svc_);
    }

    void awakened( context * ctx) noexcept {
        BOOST_ASSERT( nullptr != ctx);
        ctx->ready_link( rqueue_); /*< fiber, enqueue on ready queue >*/
    }

    context * pick_next() noexcept {
        context * ctx( nullptr);
        if ( ! rqueue_.empty() ) { /*<
            pop an item from the ready queue
        >*/
            ctx = & rqueue_.front();
            rqueue_.pop_front();
            BOOST_ASSERT( nullptr != ctx);
            BOOST_ASSERT( context::active() != ctx);
        }
        return ctx;
    }

    bool has_ready_fibers() const noexcept {
        return 0 < counter_ || ! rqueue_.empty();
    }

    void suspend_until( std::chrono::steady_clock::time_point const& abs_time) noexcept {
        if ( (std::chrono::steady_clock::time_point::max)() != abs_time) {
            suspend_timer_.expires_at( abs_time);
            suspend_timer_.async_wait([](boost::system::error_code const&){
                                        this_fiber::yield();
                                      });
        }
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
