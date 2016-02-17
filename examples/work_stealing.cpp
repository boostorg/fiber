
//          Copyright Oliver Kowalke 2015.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

#include <boost/assert.hpp>

#include <boost/fiber/all.hpp>

class work_stealing_queue {
private:
    typedef boost::fibers::scheduler::ready_queue_t rqueue_t;

    mutable std::mutex  mtx_;
    rqueue_t            rqueue_;

public:
    void push_back( boost::fibers::context * ctx) {
        BOOST_ASSERT( nullptr != ctx);
        std::unique_lock< std::mutex > lk( mtx_);
        BOOST_ASSERT( ! ctx->ready_is_linked() );
        ctx->ready_link( rqueue_);
        BOOST_ASSERT( ctx->ready_is_linked() );
    }

    boost::fibers::context * pick_next() {
        boost::fibers::context * ctx( nullptr);
        std::unique_lock< std::mutex > lk( mtx_);
        if ( ! rqueue_.empty() ) {
            ctx = & rqueue_.front();
            BOOST_ASSERT( ctx->ready_is_linked() );
            rqueue_.pop_front();
            BOOST_ASSERT( ! ctx->ready_is_linked() );
        }
        return ctx;
    }

    boost::fibers::context * steal() {
        boost::fibers::context * ctx( nullptr);
        std::unique_lock< std::mutex > lk( mtx_);
        rqueue_t::iterator e = rqueue_.end();
        rqueue_t::iterator i = std::find_if( rqueue_.begin(), e,
                                             [](boost::fibers::context const& ctx){
                                               // prevent migrating main- and dispatcher-fiber
                                               return ! ctx.is_context( boost::fibers::type::pinned_context);
                                             });
        if ( i != e) {
            ctx = & ( * i);
            rqueue_.erase( i);
            BOOST_ASSERT( ! ctx->ready_is_linked() );
            BOOST_ASSERT( ! ctx->remote_ready_is_linked() );
            BOOST_ASSERT( ! ctx->sleep_is_linked() );
            BOOST_ASSERT( ! ctx->terminated_is_linked() );
            BOOST_ASSERT( ! ctx->wait_is_linked() );
            //BOOST_ASSERT( ! ctx->worker_is_linked() );
            // attach context to current scheduler
            boost::fibers::context::active()->migrate( ctx);
        }
        return ctx;
    }

    bool empty() const noexcept {
        std::unique_lock< std::mutex > lk( mtx_);
        return rqueue_.empty();
    }

    bool has_work_items() const noexcept {
        std::unique_lock< std::mutex > lk( mtx_);
        rqueue_t::const_iterator e = rqueue_.end();
        rqueue_t::const_iterator i = std::find_if( rqueue_.begin(), e,
                                                   [](boost::fibers::context const& ctx){
                                                     return ! ctx.is_context( boost::fibers::type::pinned_context);
                                                   });
        return i != e;
    }
};

class victim_algo : public boost::fibers::thread_sched_algorithm {
private:
    typedef work_stealing_queue        rqueue_t;

    std::shared_ptr< rqueue_t > rqueue_{};

public:
    victim_algo( std::shared_ptr< rqueue_t > rqueue) :
        rqueue_( rqueue) {
    }

    virtual void awakened( boost::fibers::context * ctx) noexcept {
        BOOST_ASSERT( nullptr != ctx);
        rqueue_->push_back( ctx);
    }

    virtual boost::fibers::context * pick_next() noexcept {
        return rqueue_->pick_next();
    }

    virtual bool has_ready_fibers() const noexcept {
        return ! rqueue_->empty();
    }
};

class tief_algo : public boost::fibers::thread_sched_algorithm {
private:
    typedef boost::fibers::scheduler::ready_queue_t rqueue_t;
    typedef work_stealing_queue                     ws_rqueue_t;

    rqueue_t                        rqueue_{};
    std::shared_ptr< ws_rqueue_t >  ws_rqueue_;
    std::atomic< int >           *  count_;

public:
    tief_algo( std::shared_ptr< ws_rqueue_t > ws_rqueue, std::atomic< int > * count) :
        ws_rqueue_( ws_rqueue),
        count_( count) {
    }

    virtual void awakened( boost::fibers::context * ctx) noexcept {
        BOOST_ASSERT( nullptr != ctx);
        rqueue_.push_back( * ctx);
    }

    virtual boost::fibers::context * pick_next() noexcept {
        boost::fibers::context * ctx( nullptr);
        if ( ! rqueue_.empty() ) {
            ctx = & rqueue_.front();
            rqueue_.pop_front();
            BOOST_ASSERT( nullptr != ctx);
            if ( rqueue_.empty() ) {
                // we have no more fiber in the queue
                // try stealing a fiber from the other thread
                boost::fibers::context * stolen = ws_rqueue_->steal();
                if ( nullptr != stolen) {
                    ++( * count_);
                    rqueue_.push_back( * stolen);
                }
            }
        }
        return ctx;
    }

    virtual bool has_ready_fibers() const noexcept {
        return ! rqueue_.empty();
    }
};

boost::fibers::future< int > fibonacci( int);

int fibonacci_( int n) {
    boost::this_fiber::yield();

    int res = 1;

    if ( 0 != n && 1 != n) {
        boost::fibers::future< int > f1 = fibonacci( n - 1);
        boost::fibers::future< int > f2 = fibonacci( n - 2);

        res = f1.get() + f2.get();
    }

    return res;
}

boost::fibers::future< int > fibonacci( int n) {
    boost::fibers::packaged_task< int() > pt( std::bind( fibonacci_, n) );
    boost::fibers::future< int > f( pt.get_future() );
    boost::fibers::fiber( std::move( pt) ).detach();
    return f;
}

void thread( std::shared_ptr< work_stealing_queue > ws_queue, std::atomic< int > * count, std::atomic< bool > * fini) {
    boost::fibers::use_scheduling_algorithm< tief_algo >( ws_queue, count);

    while ( ! ( * fini) ) {
        // To guarantee progress, we must ensure that
        // threads that have work to do are not unreasonably delayed by (thief) threads
        // which are idle except for task-stealing. 
        // This call yields the thief ’s processor to another thread, allowing
        // descheduled threads to regain a processor and make progress. 
        std::this_thread::yield();
        // process stolen fibers
        boost::this_fiber::yield();
    }
}

int main() {
    std::shared_ptr< work_stealing_queue > ws_queue(
            new work_stealing_queue() );
    boost::fibers::use_scheduling_algorithm< victim_algo >( ws_queue);

    for ( int i = 0; i < 10; ++i) {
        BOOST_ASSERT( ! ws_queue->has_work_items() );
        std::atomic< int > count( 0);
        std::atomic< bool > fini( false);
        int n = 10;

        // launch a couple threads to help process them
        std::thread threads[] = {
            std::thread( thread, ws_queue, & count, & fini),
            std::thread( thread, ws_queue, & count, & fini),
            std::thread( thread, ws_queue, & count, & fini),
            std::thread( thread, ws_queue, & count, & fini),
            std::thread( thread, ws_queue, & count, & fini)
        };

        // main fiber computes fibonacci( n)
        // wait on result
        int result = fibonacci( n).get();
        BOOST_ASSERT( 89 == result);
        std::ostringstream buffer;
        buffer << "fibonacci(" << n << ") = " << result << '\n';
        std::cout << buffer.str() << std::flush;
        // set termination flag
        fini = true;

        // wait for threads to terminate
        for ( std::thread & t : threads) {
            t.join();
        }

        std::cout << count << " fibers stolen" << std::endl;
    }

    std::cout << "done." << std::endl;

    return EXIT_SUCCESS;
}
