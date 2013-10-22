//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_DEFAULT_SCHEDULER_H
#define BOOST_FIBERS_DEFAULT_SCHEDULER_H

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

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

# if defined(BOOST_MSVC)
# pragma warning(push)
# pragma warning(disable:4251 4275)
# endif

namespace boost {
namespace fibers {

class BOOST_FIBERS_DECL round_robin : public algorithm
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
    typedef std::deque< detail::fiber_base::ptr_t >     rqueue_t;

    detail::fiber_base::ptr_t   active_fiber_;
    wqueue_t                    wqueue_;
    rqueue_t                    rqueue_;

public:
    round_robin() BOOST_NOEXCEPT;

    ~round_robin() BOOST_NOEXCEPT;

    void spawn( detail::fiber_base::ptr_t const&);

    void priority( detail::fiber_base::ptr_t const&, int);

    void join( detail::fiber_base::ptr_t const&);

    detail::fiber_base::ptr_t active() BOOST_NOEXCEPT
    { return active_fiber_; }

    bool run();

    void wait( unique_lock< detail::spinlock > &);
    bool wait_until( clock_type::time_point const&,
                     unique_lock< detail::spinlock > &);

    void yield();
};

}}

# if defined(BOOST_MSVC)
# pragma warning(pop)
# endif

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DEFAULT_SCHEDULER_H
