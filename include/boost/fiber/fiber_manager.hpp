//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_FIBER_MANAGER_H
#define BOOST_FIBERS_FIBER_MANAGER_H

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/thread/lock_types.hpp> 
#include <boost/utility.hpp>

#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/main_fiber.hpp>
#include <boost/fiber/detail/spinlock.hpp>
#include <boost/fiber/detail/waiting_queue.hpp>
#include <boost/fiber/detail/worker_fiber.hpp>
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

struct sched_algorithm
{
    virtual ~sched_algorithm() {}

    virtual void awakened( detail::worker_fiber *) = 0;

    virtual detail::worker_fiber * pick_next() = 0;

    virtual void priority( detail::worker_fiber *, int) BOOST_NOEXCEPT = 0;
};

struct fiber_manager : private noncopyable
{
    fiber_manager() BOOST_NOEXCEPT;

    virtual ~fiber_manager() BOOST_NOEXCEPT;

    typedef detail::waiting_queue   wqueue_t;

    scoped_ptr< sched_algorithm >   def_algo_;
    sched_algorithm             *   sched_algo_;
    volatile wqueue_t                        wqueue_;

    clock_type::duration            wait_interval_;
    detail::worker_fiber        *   active_fiber_;
};

void fm_resume_( volatile fiber_manager *, detail:: worker_fiber *);

inline
void fm_set_sched_algo( volatile fiber_manager * fm, sched_algorithm * algo)
{
    BOOST_ASSERT( fm);
    
    fm->sched_algo_ = algo;
    //fm->def_algo_.reset();
}

void fm_spawn( volatile fiber_manager *, detail::worker_fiber *);

inline
void fm_priority( volatile fiber_manager * fm, detail::worker_fiber * f,
                  int prio) BOOST_NOEXCEPT
{
    BOOST_ASSERT( fm);
    
    fm->sched_algo_->priority( f, prio);
}

template< typename Rep, typename Period >
void fm_wait_interval( volatile fiber_manager * fm,
                       chrono::duration< Rep, Period > const& wait_interval) BOOST_NOEXCEPT
{
    BOOST_ASSERT( fm);
    
    fm->wait_interval_ = wait_interval;
}

template< typename Rep, typename Period >
chrono::duration< Rep, Period > fm_wait_interval( volatile fiber_manager * fm) BOOST_NOEXCEPT
{
    BOOST_ASSERT( fm);
    
    return fm->wait_interval_;
}

void fm_join( volatile fiber_manager *, detail::worker_fiber *);

inline
detail::worker_fiber * fm_active( volatile fiber_manager * fm) BOOST_NOEXCEPT
{
    BOOST_ASSERT( fm);
    
    return fm->active_fiber_;
}

void fm_run( volatile fiber_manager *);

void fm_wait( volatile fiber_manager *, unique_lock< detail::spinlock > &);
bool fm_wait_until( volatile fiber_manager *,
                    clock_type::time_point const&,
                    unique_lock< detail::spinlock > &);
template< typename Rep, typename Period >
bool fm_wait_for( volatile fiber_manager * fm,
                  chrono::duration< Rep, Period > const& timeout_duration,
                  unique_lock< detail::spinlock > & lk)
{
    BOOST_ASSERT( fm);
    
    return wait_until( fm, clock_type::now() + timeout_duration, lk);
}

void fm_yield( volatile fiber_manager *);

clock_type::time_point fm_next_wakeup( volatile fiber_manager *);

void fm_migrate( volatile fiber_manager *, detail::worker_fiber *);

}}

# if defined(BOOST_MSVC)
# pragma warning(pop)
# endif

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_FIBER_MANAGER_H
