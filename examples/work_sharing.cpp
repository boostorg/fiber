//          Copyright Nat Goodspeed 2015.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/fiber/all.hpp>
#include <boost/assert.hpp>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <thread>
#include <mutex>
#include <queue>
#include <string>

/*****************************************************************************
*   shared_ready_queue scheduler
*****************************************************************************/
// This simple scheduler is like round_robin, except that it shares a common
// ready queue among all participating threads. A thread participates in this
// pool by executing use_scheduling_algorithm<shared_ready_queue>() before any
// other Boost.Fiber operation.
class shared_ready_queue : public boost::fibers::sched_algorithm {
private:
    typedef std::queue<boost::fibers::context*> rqueue_t;
    // The important point about this ready queue is that it's a class static,
    // common to all instances of shared_ready_queue.
    static rqueue_t                    rqueue_;

    // so is this mutex
    static std::mutex                  mutex_;
    typedef std::unique_lock<std::mutex> lock_t;

    // Reserve a separate, thread-specific slot for this thread's main fiber.
    // It would be Bad News for thread B to retrieve and attempt to execute
    // thread A's main fiber. This slot might be empty (nullptr) or full (==
    // context::main_fiber()): pick_next() must only return the main fiber's
    // context* after it has been passed to awakened().
    boost::fibers::context*            main_fiber;

public:
    shared_ready_queue():
        main_fiber(nullptr)
    {}

    virtual void awakened( boost::fibers::context * f) {
        BOOST_ASSERT( nullptr != f);

        // recognize when we're passed this thread's main fiber
        if (f->is_main_context()) {
            // never put this thread's main fiber on the queue
            // stash it in separate slot
            main_fiber = f;
        } else {
            // ordinary fiber, enqueue on shared queue
            lock_t lock(mutex_);
            rqueue_.push( f);
        }
    }

    virtual boost::fibers::context * pick_next() {
        lock_t lock(mutex_);
        boost::fibers::context * victim;
        if ( ! rqueue_.empty() ) {
            // good, we have an item in the ready queue, pop it
            victim = rqueue_.front();
            rqueue_.pop();
            BOOST_ASSERT( nullptr != victim);
        } else {
            // nothing in the ready queue, return main_fiber
            victim = main_fiber;
            // once we've returned main_fiber, clear the slot
            main_fiber = nullptr;
        }
        return victim;
    }

    virtual std::size_t ready_fibers() const noexcept {
        lock_t lock(mutex_);
        return rqueue_.size() + (main_fiber? 1 : 0);
    }
};

shared_ready_queue::rqueue_t shared_ready_queue::rqueue_;
std::mutex shared_ready_queue::mutex_;

/*****************************************************************************
*   example fiber function
*****************************************************************************/
void whatevah(char me) {
    std::thread::id my_thread = std::this_thread::get_id();
    {
        std::ostringstream buffer;
        //buffer << "fiber " << me << " started on thread " << my_thread << '\n';
        std::cout << buffer.str() << std::flush;
    }
    for (unsigned i = 0; i < 5; ++i) {
        boost::this_fiber::yield();
        std::thread::id new_thread = std::this_thread::get_id();
        if (new_thread != my_thread) {
            my_thread = new_thread;
            std::ostringstream buffer;
            //buffer << "fiber " << me << " switched to thread " << my_thread << '\n';
            std::cout << buffer.str() << std::flush;
        }
    }
}

/*****************************************************************************
*   example thread function
*****************************************************************************/
// Wait until all running fibers have completed. This works because we happen
// to know that all example fibers use yield(), which leaves them in ready
// state. A fiber blocked on a synchronization object is invisible to
// ready_fibers().
void drain() {
    // THIS fiber is running, so won't be counted among "ready" fibers
    while (boost::fibers::ready_fibers()) {
        boost::this_fiber::yield();
    }
}

void thread() {
    boost::fibers::use_scheduling_algorithm<shared_ready_queue>();
    drain();
}

/*****************************************************************************
*   main()
*****************************************************************************/
int main( int argc, char *argv[]) {
    // use shared_ready_queue for main thread too, so we launch new fibers
    // into shared pool
    boost::fibers::use_scheduling_algorithm<shared_ready_queue>();

    for ( int i = 0; i < 10; ++i) {
        // launch a number of fibers
        for (char c : std::string("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789")) {
            boost::fibers::fiber([c](){ whatevah(c); }).detach();
        }

        // launch a couple threads to help process them
        std::thread threads[] = {
            std::thread(thread),
            std::thread(thread),
            std::thread(thread),
            std::thread(thread),
            std::thread(thread)
        };

        // drain running fibers
        drain();

        // wait for threads to terminate
        for (std::thread& t : threads) {
            t.join();
        }
    }

    return EXIT_SUCCESS;
}

