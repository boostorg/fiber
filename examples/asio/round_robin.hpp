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

typedef std::unique_lock< std::mutex > lock_t;

class round_robin : public boost::fibers::sched_algorithm {
private:
    typedef std::queue< boost::fibers::context * >  rqueue_t;

    static rqueue_t     rqueue_;
    static std::mutex   rqueue_mtx_;

    boost::asio::io_service                     &   io_svc_;
    boost::asio::steady_timer                       suspend_timer_;
    rqueue_t                                        local_queue_{};
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
        boost::asio::use_service< service >( io_svc_);
    }

    void awakened( boost::fibers::context * ctx) noexcept {
        BOOST_ASSERT( nullptr != ctx);
        if ( ctx->is_context( boost::fibers::type::pinned_context) ) { /*<
            recognize when we're passed this thread's main fiber (or an
            implicit library helper fiber): never put those on the shared
            queue
        >*/
            local_queue_.push( ctx);
            if ( ctx->is_context( boost::fibers::type::dispatcher_context) ) {
                ++counter_;
            }
        } else {
            lock_t lk(rqueue_mtx_); /*<
                worker fiber, enqueue on shared queue
            >*/
            rqueue_.push( ctx);
        }
    }

    boost::fibers::context * pick_next() noexcept {
        boost::fibers::context * ctx( nullptr);
        lock_t lk(rqueue_mtx_);
        if ( ! rqueue_.empty() ) { /*<
            pop an item from the ready queue
        >*/
            ctx = rqueue_.front();
            rqueue_.pop();
            lk.unlock();
            BOOST_ASSERT( nullptr != ctx);
            BOOST_ASSERT( boost::fibers::context::active() != ctx);
            boost::fibers::context::active()->migrate( ctx); /*<
                attach context to current scheduler via the active fiber
                of this thread; benign if the fiber already belongs to this
                thread
            >*/
        } else {
            lk.unlock();
            if ( ! local_queue_.empty() ) { /*<
                nothing in the ready queue, return main or dispatcher fiber
            >*/
                ctx = local_queue_.front();
                local_queue_.pop();
                BOOST_ASSERT ( ctx->is_context( boost::fibers::type::pinned_context) );
                if ( ctx->is_context( boost::fibers::type::dispatcher_context) ) {
                    --counter_;
                }
            }
        }
        return ctx;
    }

    bool has_ready_fibers() const noexcept {
        lock_t lock(rqueue_mtx_);
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

void run_svc( boost::asio::io_service & io_svc) {
    while ( ! io_svc.stopped() ) {
        if ( has_ready_fibers() ) {
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
}

boost::asio::io_service::id round_robin::service::id;
round_robin::rqueue_t round_robin::rqueue_{};
std::mutex round_robin::rqueue_mtx_{};

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_ASIO_ROUND_ROBIN_H
