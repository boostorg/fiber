
//          Copyright Oliver Kowalke 2015.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//

#include "boost/fiber/algo/work_stealing.hpp"

#include <random>

#include <boost/assert.hpp>

#include "boost/fiber/type.hpp"

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace algo {

std::atomic< std::uint32_t > work_stealing::counter_{ 0 };
std::vector< intrusive_ptr< work_stealing > > work_stealing::schedulers_{};

void
work_stealing::init_( std::uint32_t thread_count,
                      std::vector< intrusive_ptr< work_stealing > > & schedulers) {
    // resize array of schedulers to thread_count, initilized with nullptr
    std::vector< intrusive_ptr< work_stealing > >{ thread_count, nullptr }.swap( schedulers);
}

work_stealing::work_stealing( std::uint32_t thread_count, bool suspend) :
        id_{ counter_++ },
        distribution_{ 0, static_cast< std::uint32_t >( thread_count - 1) },
        suspend_{ suspend } {
    // initialize the array of schedulers
    static std::once_flag flag;
    std::call_once( flag, & work_stealing::init_, thread_count, std::ref( schedulers_) );
    // register pointer of this scheduler
    schedulers_[id_] = this;
}

void
work_stealing::awakened( context * ctx) noexcept {
    if ( ! ctx->is_context( type::pinned_context) ) {
        ctx->detach();
    }
    rqueue_.push( ctx);
}

context *
work_stealing::pick_next() noexcept {
    context * ctx = rqueue_.pop();
    if ( nullptr != ctx) {
        if ( ! ctx->is_context( type::pinned_context) ) {
            context::active()->attach( ctx);
        }
    } else {
        std::uint32_t id = 0;
        std::size_t count = 0, size = schedulers_.size();
        do {
            do {
                ++count;
                // random selection of one logical cpu
                // that belongs to the local NUMA node
                id = distribution_( generator_);
                // prevent stealing from own scheduler
            } while ( id == id_);
            // steal context from other scheduler
            ctx = schedulers_[id]->steal();
        } while ( nullptr == ctx && count < size);
        if ( nullptr != ctx) {
            BOOST_ASSERT( ! ctx->is_context( type::pinned_context) );
            context::active()->attach( ctx);
        }
    }
    return ctx;
}

void
work_stealing::suspend_until( std::chrono::steady_clock::time_point const& time_point) noexcept {
    if ( suspend_) {
        if ( (std::chrono::steady_clock::time_point::max)() == time_point) {
            std::unique_lock< std::mutex > lk{ mtx_ };
            cnd_.wait( lk, [this](){ return flag_; });
            flag_ = false;
        } else {
            std::unique_lock< std::mutex > lk{ mtx_ };
            cnd_.wait_until( lk, time_point, [this](){ return flag_; });
            flag_ = false;
        }
    }
}

void
work_stealing::notify() noexcept {
    if ( suspend_) {
        std::unique_lock< std::mutex > lk{ mtx_ };
        flag_ = true;
        lk.unlock();
        cnd_.notify_all();
    }
}

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
