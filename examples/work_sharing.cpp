//          Copyright Nat Goodspeed + Oliver Kowalke 2015.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <atomic>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <queue>
#include <sstream>
#include <string>
#include <thread>

#include <boost/assert.hpp>

#include <boost/fiber/all.hpp>
#include <boost/fiber/detail/autoreset_event.hpp>

static std::atomic< std::size_t > fiber_count;

/*****************************************************************************
*   shared_ready_queue scheduler
*****************************************************************************/
// This simple scheduler is like round_robin, except that it shares a common
// ready queue among all participating threads. A thread participates in this
// pool by executing use_scheduling_algorithm<shared_ready_queue>() before any
// other Boost.Fiber operation.
class shared_ready_queue : public boost::fibers::sched_algorithm {
private:
    typedef std::unique_lock< std::mutex >          lock_t;
    typedef std::queue< boost::fibers::context * >  rqueue_t;

    // The important point about this ready queue is that it's a class static,
    // common to all instances of shared_ready_queue.
    static rqueue_t                    rqueue_;

    // so is this mutex
    static std::mutex                       mtx_;

    // Reserve a separate, scheduler-specific slot for this thread's main
    // fiber. When we're passed the main fiber, stash it there instead of in
    // the shared queue: it would be Bad News for thread B to retrieve and
    // attempt to execute thread A's main fiber. This slot might be empty
    // (nullptr) or full: pick_next() must only return the main fiber's
    // context* after it has been passed to awakened().
    rqueue_t                                    local_queue_;
    boost::fibers::detail::autoreset_event      ev_;

public:
    shared_ready_queue() :
        local_queue_(),
        ev_() {
    }

    virtual void awakened( boost::fibers::context * ctx) {
        BOOST_ASSERT( nullptr != ctx);

        // recognize when we're passed this thread's main fiber
        if ( ctx->is_main_context() ) {
            // never put this thread's main fiber on the queue
            // stash it in separate slot
            local_queue_.push( ctx);
        // recognize when we're passed this thread's dispatcher fiber
        } else if ( ctx->is_dispatcher_context() ) {
            // never put this thread's main fiber on the queue
            // stash it in separate slot
            local_queue_.push( ctx);
        } else {
            // ordinary fiber, enqueue on shared queue
            ctx->worker_unlink();
            lock_t lk( mtx_);
            rqueue_.push( ctx);
        }
    }

    virtual boost::fibers::context * pick_next() {
        boost::fibers::context * ctx( nullptr);
        lock_t lk( mtx_);
        if ( ! rqueue_.empty() ) {
            // good, we have an item in the ready queue, pop it
            ctx = rqueue_.front();
            rqueue_.pop();
            lk.unlock();
            BOOST_ASSERT( nullptr != ctx);
            ctx->worker_unlink();
            ctx->set_scheduler( boost::fibers::context::active()->get_scheduler() );
        } else if ( ! local_queue_.empty() ) {
            lk.unlock();
            // nothing in the ready queue, return dispatcher_ctx_
            ctx = local_queue_.front();
            local_queue_.pop();
        }
        return ctx;
    }

    virtual bool has_ready_fibers() const noexcept {
        lock_t lock( mtx_);
        return ! rqueue_.empty() || ! local_queue_.empty();
    }

    void suspend_until( std::chrono::steady_clock::time_point const& suspend_time) {
        ev_.reset( suspend_time);
    }

    void notify() {
        ev_.set();
    }
};

shared_ready_queue::rqueue_t shared_ready_queue::rqueue_;
std::mutex shared_ready_queue::mtx_;

/*****************************************************************************
*   example fiber function
*****************************************************************************/
void whatevah( char me) {
    try {
        std::thread::id my_thread = std::this_thread::get_id();
        {
            std::ostringstream buffer;
            buffer << "fiber " << me << " started on thread " << my_thread << '\n';
            std::cout << buffer.str() << std::flush;
        }
        for ( unsigned i = 0; i < 10; ++i) {
            boost::this_fiber::yield();
            std::thread::id new_thread = std::this_thread::get_id();
            if ( new_thread != my_thread) {
                my_thread = new_thread;
                std::ostringstream buffer;
                buffer << "fiber " << me << " switched to thread " << my_thread << '\n';
                std::cout << buffer.str() << std::flush;
            }
        }
    } catch ( ... ) {
    }
    --fiber_count;
}

/*****************************************************************************
*   example thread function
*****************************************************************************/
// Wait until all running fibers have completed. This works because we happen
// to know that all example fibers use yield(), which leaves them in ready
// state. A fiber blocked on a synchronization object is invisible to
// ready_fibers().
void drain() {
    std::ostringstream buffer;
    std::cout << buffer.str() << std::flush;
    // THIS fiber is running, so won't be counted among "ready" fibers
    while ( 0 < fiber_count) {
        boost::this_fiber::yield();
    }
}

void thread() {
    std::ostringstream buffer;
    buffer << "thread started " << std::this_thread::get_id() << std::endl;
    std::cout << buffer.str() << std::flush;
    boost::fibers::use_scheduling_algorithm< shared_ready_queue >();
    drain();
}

/*****************************************************************************
*   main()
*****************************************************************************/
int main( int argc, char *argv[]) {
    std::cout << "main thread started " << std::this_thread::get_id() << std::endl;
    // use shared_ready_queue for main thread too, so we launch new fibers
    // into shared pool
    boost::fibers::use_scheduling_algorithm< shared_ready_queue >();

    for ( int i = 0; i < 10; ++i) {
        // launch a number of fibers
        for ( char c : std::string("abcdefghijklmnopqrstuvwxyz")) {
            boost::fibers::fiber([c](){ whatevah( c); }).detach();
            ++fiber_count;
        }

        // launch a couple threads to help process them
        std::thread threads[] = {
            std::thread( thread),
            std::thread( thread),
            std::thread( thread),
            std::thread( thread),
            std::thread( thread)
        };

        // drain running fibers
        drain();

        // wait for threads to terminate
        for ( std::thread & t : threads) {
            t.join();
        }
    }

    BOOST_ASSERT( 0 == fiber_count.load() );

    std::cout << "done." << std::endl;

    return EXIT_SUCCESS;
}

