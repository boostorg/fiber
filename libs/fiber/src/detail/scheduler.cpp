//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_FIBERS_SOURCE

#include <boost/fiber/detail/scheduler.hpp>

#include <memory>
#include <utility>

#include <boost/assert.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/thread/tss.hpp>

#include <boost/fiber/exceptions.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

#define RESUME_FIBER( f_) \
    BOOST_ASSERT( f_); \
    BOOST_ASSERT( ! f_->is_complete() ); \
    BOOST_ASSERT( ! f_->is_resumed() ); \
    f_->resume(); \
    BOOST_ASSERT( ! f_->is_resumed() );

namespace boost {
namespace fibers {
namespace detail {

scheduler::scheduler() :
	active_fiber_(),
	rqueue_(),
	wqueue_(),
    f_idx_( wqueue_.get< f_tag_t >() ),
    tp_idx_( wqueue_.get< tp_tag_t >() )
{}

scheduler::~scheduler()
{}

scheduler &
scheduler::instance()
{
	static thread_specific_ptr< scheduler > static_local;
	if ( ! static_local.get() ) static_local.reset( new scheduler() );
	return * static_local.get();
}

void
scheduler::spawn( fiber_base::ptr_t const& f)
{
    BOOST_ASSERT( f);
    BOOST_ASSERT( ! f->is_complete() );
    BOOST_ASSERT( f != active_fiber_);

    // TODO: strong exception-safety must be guarantied
    // use a guard for active_fiber_
    fiber_base::ptr_t tmp = active_fiber_;
    active_fiber_ = f;
    RESUME_FIBER( active_fiber_);
    active_fiber_ = tmp;
}

void
scheduler::join( fiber_base::ptr_t const& f)
{
    BOOST_ASSERT( f);
    BOOST_ASSERT( ! f->is_complete() );
    BOOST_ASSERT( f != active_fiber_);

    if ( active_fiber_)
    {
        // store active-fiber as waiting fiber in p
        // fiber_base::join() calls scheduler::wait()
        // so that active-fiber gets suspended
        f->join( active_fiber_);
        // p is complete and active-fiber is resumed
    }
    else
    {
        while ( ! f->is_complete() )
            run();
    }

    BOOST_ASSERT( f->is_complete() );
}

void
scheduler::cancel( fiber_base::ptr_t const& f)
{
    BOOST_ASSERT( f);
    BOOST_ASSERT( f != active_fiber_);

    // ignore completed fiber
    if ( f->is_complete() ) return;

    // TODO: strong exception-safety must be guarantied
    // use a guard
    fiber_base::ptr_t tmp = active_fiber_;
    active_fiber_ = f;
    // terminate fiber means unwinding its stack
    // so it becomes complete and joining fibers
    // will be notified
    active_fiber_->terminate();
    active_fiber_ = tmp;
    // erase completed fiber from waiting-queue
    f_idx_.erase( f);

    BOOST_ASSERT( f->is_complete() );
}

void
scheduler::wait( fiber_base::ptr_t const& f)
{
    BOOST_ASSERT( f);
    BOOST_ASSERT( ! f->is_complete() );
    BOOST_ASSERT( f->is_resumed() );
    BOOST_ASSERT( f == active_fiber_);

    // fiber will be added to waiting-queue
    f_idx_.insert( schedulable( f) );
    // suspend fiber
    f->suspend();
    // fiber was notified

    BOOST_ASSERT( ! f->is_complete() );
    BOOST_ASSERT( f->is_resumed() );
    BOOST_ASSERT( f == active_fiber_);
}

void
scheduler::notify( fiber_base::ptr_t const& f)
{
    BOOST_ASSERT( f);
    BOOST_ASSERT( ! f->is_complete() );
    BOOST_ASSERT( ! f->is_resumed() );
    BOOST_ASSERT( f != active_fiber_);

    // remove fiber from wait-queue
    f_idx_.erase( f);
    // push fiber at the front of the runnable-queue
    rqueue_.push_front( f);

    BOOST_ASSERT( ! f->is_complete() );
    BOOST_ASSERT( ! f->is_resumed() );
    BOOST_ASSERT( f != active_fiber_);
}

bool
scheduler::run()
{
    // get all fibers with reached dead-line and push them
    // at the front of runnable-queue
    tp_idx_t::iterator e( tp_idx_.upper_bound( chrono::system_clock::now() ) );
    for (
            tp_idx_t::iterator i( tp_idx_.begin() );
            i != e; ++i)
    { rqueue_.push_front( i->f); }
    // remove all fibers with reached dead-line
    tp_idx_.erase( tp_idx_.begin(), e);

    // pop new fiber from runnable-queue which is not complete
    // (example: fiber in runnable-queue could be canceled by active-fiber)
    fiber_base::ptr_t f;
    do
    {
        if ( rqueue_.empty() ) return false;
        f.swap( rqueue_.front() );
        rqueue_.pop_front();
        BOOST_ASSERT( f_idx_.end() == f_idx_.find( f) );
    }
    while ( f->is_complete() );
    // store previous active fiber
    f.swap( active_fiber_);
    // resume new active fiber
    RESUME_FIBER( active_fiber_);
    // restore previous active fiber
    f.swap( active_fiber_);
	return true;
}

void
scheduler::yield()
{
    BOOST_ASSERT( active_fiber_);
    BOOST_ASSERT( ! active_fiber_->is_complete() );
    BOOST_ASSERT( active_fiber_->is_resumed() );

    // yield() suspends the fiber and adds it
    // immediately to runnable-queue
    rqueue_.push_back( active_fiber_);
    active_fiber_->suspend();
    // fiber is resumed

    BOOST_ASSERT( active_fiber_->is_resumed() );
}

void
scheduler::sleep( chrono::system_clock::time_point const& abs_time)
{
    BOOST_ASSERT( active_fiber_);
    BOOST_ASSERT( ! active_fiber_->is_complete() );
    BOOST_ASSERT( active_fiber_->is_resumed() );

    if ( abs_time > chrono::system_clock::now() )
    {
        // fiber is added with a dead-line and gets suspended
        // each call of run() will check if dead-line has reached
        wqueue_.insert( schedulable( active_fiber_, abs_time) );
        active_fiber_->suspend();
        // fiber was resumed, dead-line has been reached
    }

    BOOST_ASSERT( ! active_fiber_->is_complete() );
    BOOST_ASSERT( active_fiber_->is_resumed() );
}

}}}

#undef RESUME_FIBER

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
