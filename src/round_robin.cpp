
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_wqueue_SOURCE

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
    wqueue_(),
    rqueue_mtx_(),
    rqueue_()
{}

round_robin::~round_robin()
{
    BOOST_ASSERT( ! active_fiber_);

    unique_lock< detail::spinlock > lk( rqueue_mtx_);
    BOOST_FOREACH( detail::fiber_base::ptr_t const& p, rqueue_)
    {
        p->release();
    }

    BOOST_FOREACH( detail::fiber_base::ptr_t const& p, wqueue_)
    {
        p->release();
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
    // after return from fiber::resume() the fiber is:
    // ready      -> already in requeue_
    // waiting    -> already in wqueue_
    // terminated -> not stored in round_robin/will be deleted
    //               call terminate() in order to release
    //               joining fibers
    if ( active_fiber_->is_terminated() )
        f->release();
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
        // set active fiber to state_waiting
        active_fiber_->set_waiting();
        // push active fiber to wqueue_
        wqueue_.push_back( active_fiber_);
        // add active fiber to joinig-list of f
        if ( ! f->join( active_fiber_) )
            // f must be already terminated therefore we set
            // active fiber to state_ready
            // FIXME: better state_running and no suspend
            active_fiber_->set_ready();
        // suspend active fiber until f terminates
        active_fiber_->suspend();
        // active fiber is resumed and f has teminated

        // check if fiber was interrupted
        this_fiber::interruption_point();
    }
    else
    {
        while ( ! f->is_terminated() )
        {
            //FIXME: call this_thread::yield() before ?
            //FIXME: rethrow exception from f?
            run();
        }
    }

    BOOST_ASSERT( f->is_terminated() );
}

bool
round_robin::run()
{
    // stable-sort has n*log(n) complexity if n*log(n) extra space is available
    std::size_t n = wqueue_.size();
    if ( 1 < n)
    {
        std::size_t new_capacity = n * std::log10( n) + n;
        if ( wqueue_.capacity() < new_capacity)
            wqueue_.reserve( new_capacity);

        // sort wqueue_ depending on state
        wqueue_.sort();
    }

    // check which waiting fiber should be interrupted
    // make it ready and add it to rqueue_
    std::pair< wqueue_t::iterator, wqueue_t::iterator > p =
            wqueue_.equal_range( detail::state_waiting);
    for ( wqueue_t::iterator i = p.first; i != p.second; ++i)
    {
        if ( ( * i)->interruption_requested() )
            ( * i)->set_ready();
    }

    // copy all ready fibers to rqueue_
    // and remove fibers from wqueue_
    p = wqueue_.equal_range( detail::state_ready);
    if ( p.first != p.second)
    {
        unique_lock< detail::spinlock > lk( rqueue_mtx_);
        rqueue_.insert( rqueue_.end(), p.first, p.second);
        lk.unlock();
        wqueue_.erase( p.first, p.second);
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
    // call terminate() in order to release
    // joining fibers if resumed fiber has terminated
    if ( active_fiber_->is_terminated() )
        f->release();

    return true;
}

void
round_robin::wait( unique_lock< detail::spinlock > & lk)
{
    BOOST_ASSERT( active_fiber_);
    BOOST_ASSERT( active_fiber_->is_running() );

    // set active_fiber to state_waiting
    active_fiber_->set_waiting();
    // unlock assoc. sync. primitive
    lk.unlock();
    // push active fiber to wqueue_
    wqueue_.push_back( active_fiber_);
    // suspend fiber
    active_fiber_->suspend();
    // fiber is resumed

    BOOST_ASSERT( active_fiber_->is_running() );
}

void
round_robin::yield()
{
    BOOST_ASSERT( active_fiber_);

    // set active_fiber to state_ready
    active_fiber_->set_ready();
    // suspends active fiber and adds it to wqueue_
    // Note: adding to rqueue_ could result in a raise
    // between adding to rqueue_ and calling yield another
    // thread could steel fiber from rqueue_ and resume it
    // at the same time as yield is called
    wqueue_.push_back( active_fiber_);
    // suspend fiber
    active_fiber_->suspend();
    // fiber is resumed

    BOOST_ASSERT( active_fiber_->is_running() );
}

void
round_robin::exec_in( detail::fiber_base::ptr_t const& f)
{
    BOOST_ASSERT( f);
    BOOST_ASSERT( f->is_ready() );

    wqueue_.push_back( f);
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
