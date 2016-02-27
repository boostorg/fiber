#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <mutex>
#include <numeric>
#include <queue>
#include <sstream>
#include <thread>
#include <vector>

#include <boost/fiber/all.hpp>
#include <boost/pool/pool_alloc.hpp>

#include "bind/bind_processor.hpp"

using clock_type = std::chrono::steady_clock;
using duration_type = clock_type::duration;
using time_point_type = clock_type::time_point;
using channel_type = boost::fibers::unbounded_channel< std::uint64_t >;
using allocator_type = boost::fibers::fixedsize_stack;

static bool done = false;
static std::mutex mtx{};
static boost::fibers::condition_variable_any cnd{};
using lock_t = std::unique_lock< std::mutex >;

class shared_ready_queue : public boost::fibers::sched_algorithm {
private:
    typedef std::queue< boost::fibers::context * >  rqueue_t;

    static rqueue_t     rqueue_;
    static std::mutex   rqueue_mtx_;

    rqueue_t                    local_queue_{};
    std::mutex                  mtx_{};
    std::condition_variable     cnd_{};
    bool                        flag_{ false };

public:
    virtual void awakened( boost::fibers::context * ctx) noexcept {
        if ( ctx->is_context( boost::fibers::type::pinned_context) ) {
            local_queue_.push( ctx);
        } else {
            lock_t lk(rqueue_mtx_);
            rqueue_.push( ctx);
        }
    }

    virtual boost::fibers::context * pick_next() noexcept {
        boost::fibers::context * ctx( nullptr);
        lock_t lk( rqueue_mtx_);
        if ( ! rqueue_.empty() ) {
            ctx = rqueue_.front();
            rqueue_.pop();
            lk.unlock();
            boost::fibers::context::active()->migrate( ctx);
        } else {
            lk.unlock();
            if ( ! local_queue_.empty() ) {
                ctx = local_queue_.front();
                local_queue_.pop();
            }
        }
        return ctx;
    }

    virtual bool has_ready_fibers() const noexcept {
        lock_t lock( rqueue_mtx_);
        return ! rqueue_.empty() || ! local_queue_.empty();
    }

    void suspend_until( std::chrono::steady_clock::time_point const& time_point) noexcept {
        if ( (std::chrono::steady_clock::time_point::max)() == time_point) {
            lock_t lk( mtx_);
            cnd_.wait( lk, [this](){ return flag_; });
            flag_ = false;
        } else {
            lock_t lk( mtx_);
            cnd_.wait_until( lk, time_point, [this](){ return flag_; });
            flag_ = false;
        }
    }

    void notify() noexcept {
        lock_t lk( mtx_);
        flag_ = true;
        lk.unlock();
        cnd_.notify_all();
    }
};

shared_ready_queue::rqueue_t shared_ready_queue::rqueue_{};
std::mutex shared_ready_queue::rqueue_mtx_{};

// microbenchmark
void skynet( allocator_type & salloc, channel_type & c, std::size_t num, std::size_t size, std::size_t div) {
    if ( 1 == size) {
        c.push( num);
    } else {
        channel_type rc;
        for ( std::size_t i = 0; i < div; ++i) {
            auto sub_num = num + i * size / div;
            boost::fibers::fiber{ std::allocator_arg, salloc,
                                  skynet,
                                  std::ref( salloc), std::ref( rc), sub_num, size / div, div }.detach();
        }
        std::uint64_t sum{ 0 };
        for ( std::size_t i = 0; i < div; ++i) {
            sum += rc.value_pop();
        }
        c.push( sum);
    }
}

void thread( unsigned int i) {
    bind_to_processor( i);
    boost::fibers::use_scheduling_algorithm< shared_ready_queue >();
    lock_t lk( mtx);
    cnd.wait( lk, [](){ return done; });
}

int main() {
    try {
        boost::fibers::use_scheduling_algorithm< shared_ready_queue >();
        unsigned int n = std::thread::hardware_concurrency();
        n -= 1; // this thread
        bind_to_processor( n);
        std::vector< std::thread > threads;
        for ( unsigned int i = 0; i < n; i++) {
            threads.push_back( std::thread( thread, i) );
        };
        std::size_t stack_size{ 4048 };
        std::size_t num{ 100000 };
        std::size_t div{ 10 };
        allocator_type salloc{ stack_size, 1.2*num };
        std::uint64_t result{ 0 };
        duration_type duration{ duration_type::zero() };
        boost::fibers::fiber{
                std::allocator_arg, salloc,
                [&salloc,&num,&div,&result,&duration](){
                    time_point_type start{ clock_type::now() };
                    channel_type rc;
                    for ( std::size_t i = 0; i < div; ++i) {
                        auto r = num / div;
                        auto sub_num = num + i * r;
                        boost::fibers::fiber{
                                std::allocator_arg, salloc,
                                skynet,
                                std::ref( salloc), std::ref( rc), sub_num, r, div }.detach();
                    }
                    for ( std::size_t i = 0; i < div; ++i) {
                        result += rc.value_pop();
                    }
                    duration = clock_type::now() - start;
                }}.join();
        std::cout << "Result: " << result << " in " << duration.count() / 1000000 << " ms" << std::endl;
        lock_t lk( mtx);
        done = true;
        lk.unlock();
        cnd.notify_all();
        for ( std::thread & t : threads) {
            t.join();
        }
        std::cout << "done." << std::endl;
        return EXIT_SUCCESS;
    } catch ( std::exception const& e) {
        std::cerr << "exception: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "unhandled exception" << std::endl;
    }
	return EXIT_FAILURE;
}
