
//          Copyright Oliver Kowalke 2015.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_FIBERS_ALGO_RANDOM_CHASE_LEV_H
#define BOOST_FIBERS_ALGO_RANDOM_CHASE_LEV_H

#include <condition_variable>
#include <chrono>
#include <cstddef>
#include <mutex>
#include <random>
#include <vector>

#include <boost/config.hpp>

#include <boost/fiber/algo/algorithm.hpp>
#include <boost/fiber/algo/detail/chase_lev_queue.hpp>
#include <boost/fiber/context.hpp>
#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/scheduler.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace algo {

class random_chase_lev : public algorithm {
private:
    typedef scheduler::ready_queue_t lqueue_t;

    static std::vector< random_chase_lev * >        schedulers_;
    static std::mutex                               schedulers_mutex_;

    std::size_t                                     idx_;
    std::random_device                              rd_device_{};
    std::minstd_rand                                generator_;
    detail::chase_lev_queue                         rqueue_{};
    lqueue_t                                        lqueue_{};
    std::mutex                                      mtx_{};
    std::condition_variable                         cnd_{};
    bool                                            flag_{ false };
    bool                                            suspend_;

public:
    random_chase_lev( bool suspend = false);

	random_chase_lev( random_chase_lev const&) = delete;
	random_chase_lev( random_chase_lev &&) = delete;

	random_chase_lev & operator=( random_chase_lev const&) = delete;
	random_chase_lev & operator=( random_chase_lev &&) = delete;

    void awakened( context * ctx) noexcept;

    context * pick_next() noexcept;

    context * steal() noexcept {
        return rqueue_.steal();
    }

    bool has_ready_fibers() const noexcept {
        return ! rqueue_.empty() || ! lqueue_.empty();
    }

	void suspend_until( std::chrono::steady_clock::time_point const& time_point) noexcept;

	void notify() noexcept;
};

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_ALGO_RANDOM_CHASE_LEV_H
