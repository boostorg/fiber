//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_DETAIL_SCHEDULER_H
#define BOOST_FIBERS_DETAIL_SCHEDULER_H

#if defined(__APPLE__) && defined(BOOST_HAS_PTHREADS)
#include <pthread.h>                // pthread_key_create, pthread_[gs]etspecific
#endif

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

class scheduler;

// thread_local_ptr was a contribution from
// Nat Goodspeed
#if defined(__APPLE__) && defined(BOOST_HAS_PTHREADS)
class thread_local_ptr : private noncopyable
{
private:
    struct dummy
    { void nonnull() {} };

    typedef void ( dummy::*safe_bool)();

    ::pthread_key_t     key_;

public:
    thread_local_ptr() BOOST_NOEXCEPT
    { BOOST_ASSERT( ! ::pthread_key_create( & key_, 0) ); }

    scheduler * get() const BOOST_NOEXCEPT
    { return static_cast< scheduler * >( ::pthread_getspecific( key_) ); }

    thread_local_ptr & operator=( scheduler * ptr) BOOST_NOEXCEPT
    {
        ::pthread_setspecific( key_, ptr);
        return * this;
    }

    scheduler & operator*() const BOOST_NOEXCEPT
    { return * get(); }

    scheduler * operator->() const BOOST_NOEXCEPT
    { return get(); }

    operator scheduler * () const BOOST_NOEXCEPT
    { return get(); }

    operator safe_bool() const BOOST_NOEXCEPT
    { return get() ? 0 : & dummy::nonnull; }

    bool operator!() const BOOST_NOEXCEPT
    { return ! get(); }

    bool operator==( thread_local_ptr const& other) BOOST_NOEXCEPT
    { return this->get() == other.get(); }

    bool operator!=( thread_local_ptr const& other) BOOST_NOEXCEPT
    { return ! ( * this == other); }
};
#endif

class BOOST_FIBERS_DECL scheduler : private noncopyable
{
private:
    struct schedulable
    {
        fiber_base::ptr_t                   f;
        chrono::system_clock::time_point    tp;

        schedulable( fiber_base::ptr_t const& f_) :
            f( f_), tp( (chrono::system_clock::time_point::max)() )
        { BOOST_ASSERT( f); }

        schedulable(
                fiber_base::ptr_t const& f_,
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

#if defined(_MSC_VER) || defined(__BORLANDC__) || defined(__DMC__) || \
    (defined(__ICC) && defined(BOOST_WINDOWS))
    static __declspec(thread) scheduler     *   instance_;
#elif defined(BOOST_MAC_PTHREADS)
    static thread_local_ptr                     instance_;
#else
    static __thread scheduler               *   instance_;
#endif

    fiber_base::ptr_t       active_fiber_;
    rqueue_t                rqueue_;
    wqueue_t                wqueue_;
    f_idx_t             &   f_idx_;
    tp_idx_t            &   tp_idx_;

    scheduler();

public:
    static scheduler & instance();

    void spawn( fiber_base::ptr_t const&);

    void join( fiber_base::ptr_t const&);

    void cancel( fiber_base::ptr_t const&);

    void notify( fiber_base::ptr_t const&);

    fiber_base::ptr_t active() BOOST_NOEXCEPT
    { return active_fiber_; }

    void sleep( chrono::system_clock::time_point const& abs_time);

    bool run();

    void wait();

    void yield();

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
