
//          Copyright Oliver Kowalke / Nat Goodspeed 2015.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/fiber/algorithm.hpp"

#include "boost/fiber/context.hpp"

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

void
thread_sched_algorithm::suspend_until( std::chrono::steady_clock::time_point const& time_point) noexcept {
    if ( (std::chrono::steady_clock::time_point::max)() == time_point) {
        // Some implementations of std::condition_variable::wait_until() don't
        // deal well with time_point == time_point::max(), so detect that case
        // and call vanilla wait() instead, with no timeout.
        std::unique_lock< std::mutex > lk( mtx_);
        cnd_.wait( lk, [&](){ return flag_; });
        flag_ = false;
    } else {
        std::unique_lock< std::mutex > lk( mtx_);
        cnd_.wait_until( lk, time_point, [&](){ return flag_; });
        flag_ = false;
    }
}

void
thread_sched_algorithm::notify() noexcept {
    std::unique_lock< std::mutex > lk( mtx_);
    flag_ = true;
    lk.unlock();
    cnd_.notify_all();
}

//static
fiber_properties *
sched_algorithm_with_properties_base::get_properties( context * ctx) noexcept {
    return ctx->get_properties();
}

//static
void
sched_algorithm_with_properties_base::set_properties( context * ctx, fiber_properties * props) noexcept {
    ctx->set_properties( props);
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
