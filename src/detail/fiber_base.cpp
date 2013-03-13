
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_FIBERS_SOURCE

#include <boost/fiber/detail/fiber_base.hpp>

#include <boost/exception_ptr.hpp>
#include <boost/foreach.hpp>
#include <boost/thread/locks.hpp>

#include <boost/fiber/detail/scheduler.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace detail {

const int fiber_base::READY = 0;
const int fiber_base::RUNNING = 1;
const int fiber_base::WAITING = 2;
const int fiber_base::TERMINATED = 3;

fiber_base::fiber_base( context::fcontext_t * callee, bool preserve_fpu) :
    state_( READY),
    flags_( 0),
    priority_( 0),
    caller_(),
    callee_( callee),
    except_(),
    waiting_mtx_(),
    waiting_()
{ if ( preserve_fpu) flags_ |= flag_preserve_fpu; }

fiber_base::~fiber_base()
{
    BOOST_ASSERT( is_terminated() );
    BOOST_ASSERT( waiting_.empty() );
}

void
fiber_base::resume()
{
    BOOST_ASSERT( is_running() );

    context::jump_fcontext( & caller_, callee_, 0, preserve_fpu() );

    if ( has_exception() ) rethrow();
}

void
fiber_base::suspend()
{
    context::jump_fcontext( callee_, & caller_, 0, preserve_fpu() );

    BOOST_ASSERT( is_running() );

    if ( unwind_requested() ) throw forced_unwind();
}

void
fiber_base::release()
{
    BOOST_ASSERT( is_terminated() );

    std::vector< ptr_t > waiting;

    // get all waiting fibers
    unique_lock< spinlock > lk( waiting_mtx_);
    waiting.swap( waiting_);
    lk.unlock();

    // notify all waiting fibers
    BOOST_FOREACH( fiber_base::ptr_t p, waiting)
    { p->set_ready(); }
}

bool
fiber_base::join( ptr_t const& p)
{
    // protect against concurrent access to waiting_
    unique_lock< spinlock > lk( waiting_mtx_);
    if ( is_terminated() ) return false;
    waiting_.push_back( p);
    return true;
}

void
fiber_base::rethrow() const
{
    BOOST_ASSERT( has_exception() );

    rethrow_exception( except_);
}

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
