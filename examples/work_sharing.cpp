//          Copyright Nat Goodspeed + Oliver Kowalke 2015.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

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

#include "barrier.hpp"

static std::size_t fiber_count{ 0 };
static std::mutex mtx_count{};
static boost::fibers::condition_variable_any cnd_count{};
typedef std::unique_lock< std::mutex > lock_count;

/*****************************************************************************
*   shared_ready_queue scheduler
*****************************************************************************/
class shared_ready_queue : public boost::fibers::sched_algorithm {
private:
    typedef std::unique_lock< std::mutex >          lock_t;
    typedef std::queue< boost::fibers::context * >  rqueue_t;

    static rqueue_t     rqueue_;
    static std::mutex   rqueue_mtx_;

    rqueue_t                                local_queue_{};
    boost::fibers::detail::autoreset_event  ev_{};

public:
//[awakened_ws
    virtual void awakened( boost::fibers::context * ctx) noexcept {
        BOOST_ASSERT( nullptr != ctx);

        if ( ctx->is_main_context() ) { /*<
            recognize when we're passed this thread's main fiber
            never put this thread's main fiber on the queue stash
            it in separate slot
        >*/
            local_queue_.push( ctx);
        } else if ( ctx->is_dispatcher_context() ) { /*<
            recognize when we're passed this thread's dispatcher fiber
            never put this thread's main fiber on the queue
            stash it in separate slot
        >*/
            local_queue_.push( ctx);
        } else {
            lock_t lk(rqueue_mtx_); /*<
                worker fiber, enqueue on shared queue
            >*/
            rqueue_.push( ctx);
        }
    }
//]
//[pick_next_ws
    virtual boost::fibers::context * pick_next() noexcept {
        boost::fibers::context * ctx( nullptr);
        lock_t lk(rqueue_mtx_);
        if ( ! rqueue_.empty() ) { /*<
            pop an item from the ready queue
        >*/
            ctx = rqueue_.front();
            rqueue_.pop();
            lk.unlock();
            BOOST_ASSERT( nullptr != ctx);
            boost::fibers::context::active()->migrate( ctx); /*<
                attach context to current scheduler via the active fiber
                of this thread
            >*/
        } else {
            lk.unlock();
            if ( ! local_queue_.empty() ) { /*<
                nothing in the ready queue, return main or dispatcher fiber
            >*/
                ctx = local_queue_.front();
                local_queue_.pop();
            }
        }
        return ctx;
    }
//]

    virtual bool has_ready_fibers() const noexcept {
        lock_t lock(rqueue_mtx_);
        return ! rqueue_.empty() || ! local_queue_.empty();
    }

    void suspend_until( std::chrono::steady_clock::time_point const& suspend_time) noexcept {
        ev_.reset( suspend_time);
    }

    void notify() noexcept {
        ev_.set();
    }
};

shared_ready_queue::rqueue_t shared_ready_queue::rqueue_{};
std::mutex shared_ready_queue::rqueue_mtx_{};

/*****************************************************************************
*   example fiber function
*****************************************************************************/
//[fiber_fn_ws
void whatevah( char me) {
    try {
        std::thread::id my_thread = std::this_thread::get_id(); /*< get ID of initial thread >*/
        {
            std::ostringstream buffer;
            buffer << "fiber " << me << " started on thread " << my_thread << '\n';
            std::cout << buffer.str() << std::flush;
        }
        for ( unsigned i = 0; i < 10; ++i) { /*< loop ten time >*/
            boost::this_fiber::yield(); /*< yield this fiber >*/
            std::thread::id new_thread = std::this_thread::get_id(); /*< get ID of current thread >*/
            if ( new_thread != my_thread) { /*< test if fiber was migrated to another thread >*/
                my_thread = new_thread;
                std::ostringstream buffer;
                buffer << "fiber " << me << " switched to thread " << my_thread << '\n';
                std::cout << buffer.str() << std::flush;
            }
        }
    } catch ( ... ) {
    }
    lock_count lk( mtx_count);
    if ( 0 == --fiber_count) { /*< Decrement fiber counter for each completed fiber. >*/
        lk.unlock();
        cnd_count.notify_all(); /*< Notify all fibers waiting on `cnd_count`. >*/
    }
}
//]

/*****************************************************************************
*   example thread function
*****************************************************************************/
//[thread_fn_ws
void thread( barrier * b) {
    std::ostringstream buffer;
    buffer << "thread started " << std::this_thread::get_id() << std::endl;
    std::cout << buffer.str() << std::flush;
    boost::fibers::use_scheduling_algorithm< shared_ready_queue >(); /*<
        Install the scheduling algorithm `shared_ready_queue` in order to
        join the work sharing.
    >*/
    b->wait(); /*< wait on other threads >*/
    lock_count lk( mtx_count);
    if ( 0 < fiber_count) { /*< no spurious wakeup >*/
        cnd_count.wait( lk); /*<
            Suspend main fiber and resume worker fibers in the meanwhile.
            Main fiber gets resumed (e.g returns from `condition-variable_any::wait()`)
            if all worker fibers are complete.
        >*/
    }
    BOOST_ASSERT( 0 == fiber_count);
}
//]

/*****************************************************************************
*   main()
*****************************************************************************/
int main( int argc, char *argv[]) {
    std::cout << "main thread started " << std::this_thread::get_id() << std::endl;
//[main_ws
    boost::fibers::use_scheduling_algorithm< shared_ready_queue >(); /*<
        Install the scheduling algorithm `shared_ready_queue` in the main thread
        too, so each new fiber gets launched into the shared pool.
    >*/

    for ( char c : std::string("abcdefghijklmnopqrstuvwxyz")) { /*<
        Launch a number of worker fibers; each worker fiber picks-up a character
        that is passed as parameter to fiber-function `whatevah`.
        Each worker fiber gets detached, e.g. `shared_ready_queue` takes care
        of fibers life-time.
    >*/
        boost::fibers::fiber([c](){ whatevah( c); }).detach();
        ++fiber_count; /*< Increment fiber counter for each new fiber. >*/
    }
    barrier b( 4);
    std::thread threads[] = { /*<
        Launch a couple of threads that join the work sharing.
    >*/
        std::thread( thread, & b),
        std::thread( thread, & b),
        std::thread( thread, & b)
    };
    b.wait(); /*< wait on other threads >*/
    lock_count lk( mtx_count);
    if ( 0 < fiber_count) { /*< no spurious wakeup >*/
        cnd_count.wait( lk); /*<
            Suspend main fiber and resume worker fibers in the meanwhile.
            Main fiber gets resumed (e.g returns from `condition-variable_any::wait()`)
            if all worker fibers are complete.
        >*/
    }
    lk.unlock(); /*<
        Releasing lock of mtx_count is required before joining the threads, othwerwise
        the other threads would be blocked inside condition_variable::wait() and
        would never return (deadlock).
    >*/
    BOOST_ASSERT( 0 == fiber_count);
    for ( std::thread & t : threads) { /*< wait for threads to terminate >*/
        t.join();
    }
//]
    std::cout << "done." << std::endl;
    return EXIT_SUCCESS;
}
