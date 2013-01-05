//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_DEFAULT_SCHEDULER_H
#define BOOST_FIBERS_DEFAULT_SCHEDULER_H

#include <deque>
#include <set>
#include <vector>

#include <boost/assert.hpp>
#include <boost/chrono/system_clocks.hpp>
#include <boost/config.hpp>

#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/container.hpp>
#include <boost/fiber/detail/spin_mutex.hpp>
#include <boost/fiber/fiber.hpp>
#include <boost/fiber/algorithm.hpp>

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
        detail::fiber_base::ptr_t           f;
        chrono::system_clock::time_point    tp;

        schedulable( detail::fiber_base::ptr_t const& f_) :
            f( f_), tp( (chrono::system_clock::time_point::max)() )
        { BOOST_ASSERT( f); }

        schedulable(
                detail::fiber_base::ptr_t const& f_,
                chrono::system_clock::time_point const& tp_) :
            f( f_), tp( tp_)
        {
            BOOST_ASSERT( f);
            BOOST_ASSERT( (chrono::system_clock::time_point::max)() != tp);
        }

        bool operator<( schedulable const& other)
        { return tp < other.tp; }
    };

    typedef detail::container<>                         container_t;
    typedef std::deque< detail::fiber_base::ptr_t >     rqueue_t;
    typedef std::set< schedulable >                     sleeping_t;

    detail::fiber_base::ptr_t   active_fiber_;
    container_t                 fibers_;
    rqueue_t                    rqueue_;
    sleeping_t                  sleeping_;

    void process_fibers_();

public:
    round_robin() BOOST_NOEXCEPT;

    ~round_robin() BOOST_NOEXCEPT;

    void spawn( detail::fiber_base::ptr_t const&);

    void priority( detail::fiber_base::ptr_t const&, int);

    void join( detail::fiber_base::ptr_t const&);

    void cancel( detail::fiber_base::ptr_t const&);

    detail::fiber_base::ptr_t active() BOOST_NOEXCEPT
    { return active_fiber_; }

    void sleep( chrono::system_clock::time_point const&);

    bool run();

    void wait( detail::spin_mutex::scoped_lock &);

    void yield();

    void migrate_to( detail::fiber_base::ptr_t const&);

    detail::fiber_base::ptr_t migrate_from();
};

}}

# if defined(BOOST_MSVC)
# pragma warning(pop)
# endif

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DEFAULT_SCHEDULER_H
