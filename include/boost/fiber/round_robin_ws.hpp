//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_DEFAULT_SCHEDULER_WS_H
#define BOOST_FIBERS_DEFAULT_SCHEDULER_WS_H

#include <deque>

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/thread/lock_types.hpp> 

#include <boost/fiber/algorithm.hpp>
#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/fiber_base.hpp>
#include <boost/fiber/detail/spinlock.hpp>
#include <boost/fiber/detail/ws_queue.hpp>
#include <boost/fiber/fiber.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

# if defined(BOOST_MSVC)
# pragma warning(push)
# pragma warning(disable:4251 4275)
# endif

namespace boost {
namespace fibers {

class BOOST_FIBERS_DECL round_robin_ws : public algorithm
{
private:
    struct schedulable
    {
        detail::fiber_base::ptr_t   f;
        clock_type::time_point      tp;

        schedulable( detail::fiber_base::ptr_t const& f_,
                     clock_type::time_point const& tp_ =
                        (clock_type::time_point::max)() ) :
            f( f_), tp( tp_)
        { BOOST_ASSERT( f); }
    };

    typedef std::deque< schedulable >                   wqueue_t;

    detail::fiber_base::ptr_t   active_fiber_;
    wqueue_t                    wqueue_;
    detail::ws_queue            rqueue_;

public:
    round_robin_ws() BOOST_NOEXCEPT;

    ~round_robin_ws() BOOST_NOEXCEPT;

    void spawn( detail::fiber_base::ptr_t const&);

    void priority( detail::fiber_base::ptr_t const&, int) BOOST_NOEXCEPT;

    void join( detail::fiber_base::ptr_t const&);

    detail::fiber_base::ptr_t active() BOOST_NOEXCEPT
    { return active_fiber_; }

    bool run();

    void wait( unique_lock< detail::spinlock > &);
    bool wait_until( clock_type::time_point const&,
                     unique_lock< detail::spinlock > &);

    void yield();

    fiber steal_from();
    void migrate_to( fiber const&);
};

}}

# if defined(BOOST_MSVC)
# pragma warning(pop)
# endif

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DEFAULT_SCHEDULER_WS_H
