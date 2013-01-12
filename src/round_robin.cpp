//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_FIBERS_SOURCE

#include <boost/fiber/round_robin.hpp>

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>

#include <boost/assert.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/scope_exit.hpp>
#include <boost/thread/locks.hpp>

#include <boost/fiber/exceptions.hpp>
#include <boost/fiber/interruption.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

round_robin::round_robin() :
    active_fiber_(),
    fibers_(),
    rqueue_mtx_(),
    rqueue_()
{}

round_robin::~round_robin()
{
    BOOST_ASSERT( ! active_fiber_);

    unique_lock< detail::spinlock > lk( rqueue_mtx_);
    rqueue_.clear();
    lk.unlock();

    BOOST_FOREACH( detail::fiber_base::ptr_t const& p, fibers_)
    {
        p->request_interruption( true);
        while ( ! fibers_.empty() ) run();
    }
}

void
round_robin::spawn( detail::fiber_base::ptr_t const& f)
{
    BOOST_ASSERT( f);
    BOOST_ASSERT( ! f->is_terminated() );
    BOOST_ASSERT( f != active_fiber_);

    detail::fiber_base::ptr_t tmp = active_fiber_;
    BOOST_SCOPE_EXIT( & tmp, & active_fiber_) {
        active_fiber_ = tmp;
    } BOOST_SCOPE_EXIT_END
    active_fiber_ = f;
    active_fiber_->set_running();
    active_fiber_->resume();
    if ( ! f->is_terminated() )
    {
        fibers_.push_back( f);
    }
}

void
round_robin::priority( detail::fiber_base::ptr_t const& f, int prio)
{
    BOOST_ASSERT( f);

    f->priority( prio);
}

void
round_robin::join( detail::fiber_base::ptr_t const& f)
{
    BOOST_ASSERT( f);
    BOOST_ASSERT( f != active_fiber_);

    if ( f->is_terminated() ) return;

    if ( active_fiber_)
    {
        // set active_fiber to state_waiting
        active_fiber_->set_waiting();
        // add active_fiber_ to joinig-list of f
        if ( ! f->join( active_fiber_) )
            //FIXME: in-performant -> better state changed to running
            active_fiber_->set_ready();
        // suspend active-fiber until f terminates
        active_fiber_->suspend();
        // fiber is resumed
        // f has teminated and active-fiber is resumed

        // check if fiber was interrupted
        this_fiber::interruption_point();
    }
    else
    {
        while ( ! f->is_terminated() )
        {
            //FIXME: this_thread::yield() ?
            run();
        }
    }

    BOOST_ASSERT( f->is_terminated() );
}

bool
round_robin::run()
{
    // stable-sort has n*log(n) complexity if n*log(n) extra space is available
    std::size_t n = fibers_.size();
    if ( 1 < n)
    {
        std::size_t new_capacity = n * std::log10( n) + n;
        if ( fibers_.capacity() < new_capacity)
            fibers_.reserve( new_capacity);

        // sort fibers_ depending on state
        fibers_.sort();
    }

    // check which waiting fdiber should be interrupted
    // make it ready and add it to rqueue_
    std::pair< container_t::iterator, container_t::iterator > p =
            fibers_.equal_range( detail::state_waiting);
    for ( container_t::iterator i = p.first; i != p.second; ++i)
    {
        if ( ( * i)->interruption_requested() )
        {
            ( * i)->set_ready();
            unique_lock< detail::spinlock > lk( rqueue_mtx_);
            rqueue_.push_back( * i);
        }
    }

    // copy all ready fibers to rqueue_
    p = fibers_.equal_range( detail::state_ready);
    if ( p.first != p.second)
    {
        unique_lock< detail::spinlock > lk( rqueue_mtx_);
        rqueue_.insert( rqueue_.end(), p.first, p.second);
    }

    // remove all terminated fibers from fibers_
    p = fibers_.equal_range( detail::state_terminated);
    for ( container_t::iterator i = p.first; i != p.second; ++i)
    {
        ( * i)->terminate();
        fibers_.erase( i);
    }

    {
        unique_lock< detail::spinlock > lk( rqueue_mtx_);
        if ( rqueue_.empty() ) return false;
    }

    // pop new fiber from runnable-queue which is not complete
    // (example: fiber in runnable-queue could be canceled by active-fiber)
    detail::fiber_base::ptr_t f;
    do
    {
        unique_lock< detail::spinlock > lk( rqueue_mtx_);
        if ( rqueue_.empty() ) return false;
        f.swap( rqueue_.front() );
        rqueue_.pop_front();
    }
    while ( ! f->is_ready() );

    detail::fiber_base::ptr_t tmp = active_fiber_;
    BOOST_SCOPE_EXIT( & tmp, & active_fiber_) {
        active_fiber_ = tmp;
    } BOOST_SCOPE_EXIT_END
    active_fiber_ = f;
    // resume new active fiber
    active_fiber_->set_running();
    active_fiber_->resume();

    return true;
}

void
round_robin::wait( unique_lock< detail::spinlock > & lk)
{
    BOOST_ASSERT( active_fiber_);
    BOOST_ASSERT( active_fiber_->is_running() );

    // set active_fiber to state_waiting
    active_fiber_->set_waiting();
    // unlock Lock assoc. with sync. primitive
    lk.unlock();
    // suspend fiber
    active_fiber_->suspend();
    // fiber is resumed

    BOOST_ASSERT( active_fiber_->is_running() );
}

void
round_robin::yield()
{
    BOOST_ASSERT( active_fiber_);
    BOOST_ASSERT( active_fiber_->is_running() );

    // yield() suspends the fiber and adds it
    // immediately to ready-queue
    unique_lock< detail::spinlock > lk( rqueue_mtx_);
    rqueue_.push_back( active_fiber_);
    lk.unlock();
    // set active_fiber to state_ready
    active_fiber_->set_ready();
    // suspend fiber
    active_fiber_->yield();
    // fiber is resumed

    BOOST_ASSERT( active_fiber_->is_running() );
}

void
round_robin::exec_in( detail::fiber_base::ptr_t const& f)
{
    BOOST_ASSERT( f);
    BOOST_ASSERT( f->is_ready() );

    unique_lock< detail::spinlock > lk( rqueue_mtx_);
    rqueue_.push_back( f);
}

detail::fiber_base::ptr_t
round_robin::steel_from()
{
    detail::fiber_base::ptr_t f;

    unique_lock< detail::spinlock > lk( rqueue_mtx_);
    if ( ! rqueue_.empty() )
    {
        f.swap( rqueue_.back() );
        rqueue_.pop_back();
        BOOST_ASSERT( f->is_ready() );
    }

    return f;
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
