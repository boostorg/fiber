//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_DEFAULT_SCHEDULER_H
#define BOOST_FIBERS_DEFAULT_SCHEDULER_H

#include <deque>

#include <boost/assert.hpp>
#include <boost/chrono/system_clocks.hpp>
#include <boost/config.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>

#include <boost/fiber/detail/config.hpp>
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
    };

    struct f_tag_t {};
    struct tp_tag_t {};

    typedef multi_index::multi_index_container<
        schedulable,
        multi_index::indexed_by<
            multi_index::ordered_unique<
                multi_index::tag< f_tag_t >,
                multi_index::member< schedulable, detail::fiber_base::ptr_t, & schedulable::f >
            >,
            multi_index::ordered_non_unique<
                multi_index::tag< tp_tag_t >,
                multi_index::member< schedulable, chrono::system_clock::time_point, & schedulable::tp >
            >
        >
    >                                                   wqueue_t;
    typedef wqueue_t::index< f_tag_t >::type            f_idx_t;
    typedef wqueue_t::index< tp_tag_t >::type           tp_idx_t;
    typedef std::deque< detail::fiber_base::ptr_t >     rqueue_t;

    detail::fiber_base::ptr_t   active_fiber_;
    rqueue_t                    rqueue_;
    wqueue_t                    wqueue_;
    f_idx_t                 &   f_idx_;
    tp_idx_t                &   tp_idx_;

public:
    typedef rqueue_t::iterator          iterator;
    typedef rqueue_t::const_iterator    const_iterator;

    round_robin() BOOST_NOEXCEPT;

    iterator begin()
    { return rqueue_.begin(); }

    const_iterator begin() const
    { return rqueue_.begin(); }

    iterator end()
    { return rqueue_.end(); }

    const_iterator end() const
    { return rqueue_.end(); }

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
