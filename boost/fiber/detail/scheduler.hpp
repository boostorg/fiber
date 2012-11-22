//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_DETAIL_SCHEDULER_H
#define BOOST_FIBERS_DETAIL_SCHEDULER_H

#include <cstddef>
#include <deque>
#include <vector>

#include <boost/chrono/system_clocks.hpp>
#include <boost/config.hpp>
#include <boost/function.hpp>
#include <boost/move/move.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/optional.hpp>
#include <boost/utility.hpp>

#include <boost/fiber/detail/config.hpp>
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
namespace detail {

class BOOST_FIBERS_DECL scheduler : private noncopyable
{
private:
    struct schedulable
    {
        fiber_base::ptr_t                   f;
        chrono::system_clock::time_point    tp;

        schedulable( fiber_base::ptr_t const& f_) :
            f( f_), tp( chrono::system_clock::time_point::max() )
        { BOOST_ASSERT( f); }

        schedulable(
                fiber_base::ptr_t const& f_,
                chrono::system_clock::time_point const& tp_) :
            f( f_), tp( tp_)
        {
            BOOST_ASSERT( f);
            BOOST_ASSERT( chrono::system_clock::time_point::max() != tp);
        }
    };

    struct f_tag_t {};
    struct tp_tag_t {};

    typedef multi_index::multi_index_container<
        schedulable,
        multi_index::indexed_by<
            multi_index::ordered_unique<
                multi_index::tag< f_tag_t >,
                multi_index::member< schedulable, fiber_base::ptr_t, & schedulable::f >
            >,
            multi_index::ordered_non_unique<
                multi_index::tag< tp_tag_t >,
                multi_index::member< schedulable, chrono::system_clock::time_point, & schedulable::tp >
            >
        >
    >                                           wqueue_t;
    typedef wqueue_t::index< f_tag_t >::type    f_idx_t;
    typedef wqueue_t::index< tp_tag_t >::type   tp_idx_t;

    typedef std::deque< fiber_base::ptr_t >     rqueue_t;

    fiber_base::ptr_t       active_fiber_;
    rqueue_t                rqueue_;
    wqueue_t                wqueue_;
    f_idx_t             &   f_idx_;
    tp_idx_t            &   tp_idx_;

    scheduler();

    void resume_();

public:
    static scheduler & instance();

    void spawn( fiber_base::ptr_t const&);

    void join( fiber_base::ptr_t const&);

    void cancel( fiber_base::ptr_t const&);

    void wait( fiber_base::ptr_t const&);

    void notify( fiber_base::ptr_t const&);

    bool run();

    void yield();

    fiber_base::ptr_t active()
    { return active_fiber_; }

    void sleep( chrono::system_clock::time_point const& abs_time);

    ~scheduler();
};

}}}

# if defined(BOOST_MSVC)
# pragma warning(pop)
# endif

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DETAIL_SCHEDULER_H
