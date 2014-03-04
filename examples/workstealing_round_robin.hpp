//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef WORKSTEALING_ROND_ROBIN_H
#define WORKSTEALING_ROND_ROBIN_H

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
#include <boost/fiber/detail/main_notifier.hpp>
#include <boost/fiber/detail/notify.hpp>
#include <boost/fiber/detail/spinlock.hpp>
#include <boost/fiber/fiber.hpp>

#include "ws_queue.hpp"

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

# if defined(BOOST_MSVC)
# pragma warning(push)
# pragma warning(disable:4251 4275)
# endif

class workstealing_round_robin : public boost::fibers::algorithm
{
private:
    struct schedulable
    {
        boost::fibers::detail::fiber_base::ptr_t   f;
        boost::fibers::clock_type::time_point      tp;

        schedulable( boost::fibers::detail::fiber_base::ptr_t const& f_,
                     boost::fibers::clock_type::time_point const& tp_ =
                        (boost::fibers::clock_type::time_point::max)() ) :
            f( f_), tp( tp_)
        { BOOST_ASSERT( f); }
    };

    typedef std::deque< schedulable >                   wqueue_t;

    boost::fibers::detail::fiber_base::ptr_t   active_fiber_;
    wqueue_t                    wqueue_;
    ws_queue            rqueue_;
    boost::fibers::detail::main_notifier       mn_;

public:
    workstealing_round_robin() BOOST_NOEXCEPT;

    ~workstealing_round_robin() BOOST_NOEXCEPT;

    void spawn( boost::fibers::detail::fiber_base::ptr_t const&);

    void priority( boost::fibers::detail::fiber_base::ptr_t const&, int) BOOST_NOEXCEPT;

    void join( boost::fibers::detail::fiber_base::ptr_t const&);

    boost::fibers::detail::fiber_base::ptr_t active() BOOST_NOEXCEPT
    { return active_fiber_; }

    bool run();

    void wait( boost::unique_lock< boost::fibers::detail::spinlock > &);
    bool wait_until( boost::fibers::clock_type::time_point const&,
                     boost::unique_lock< boost::fibers::detail::spinlock > &);

    void yield();

    boost::fibers::detail::fiber_base::id get_main_id()
    { return boost::fibers::detail::fiber_base::id( boost::fibers::detail::main_notifier::make_pointer( mn_) ); }

    boost::fibers::detail::notify::ptr_t get_main_notifier()
    { return boost::fibers::detail::notify::ptr_t( new boost::fibers::detail::main_notifier() ); }

    boost::fibers::fiber steal_from();
    void migrate_to( boost::fibers::fiber const&);
};

# if defined(BOOST_MSVC)
# pragma warning(pop)
# endif

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // WORKSTEALING_ROND_ROBIN_H
