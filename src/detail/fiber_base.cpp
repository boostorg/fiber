
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

fiber_base::fiber_base( context::fcontext_t * callee, bool preserve_fpu) :
    use_count_( 0),
    state_( state_ready),
    flags_( 0),
    priority_( 0),
    caller_(),
    callee_( callee),
    except_(),
    joining_mtx_(),
    joining_()
{ if ( preserve_fpu) flags_ |= flag_preserve_fpu; }

void
fiber_base::resume()
{
    context::jump_fcontext( & caller_, callee_, 0, preserve_fpu() );

    if ( has_exception() ) rethrow();
}

void
fiber_base::suspend()
{
    context::jump_fcontext( callee_, & caller_, 0, preserve_fpu() );

    if ( unwind_requested() ) throw forced_unwind();
}

void
fiber_base::release()
{
    if ( ! is_terminated() ) unwind_stack();

    // set all waiting fibers in joining_ to state_ready
    // so they can be resumed
    // protect against concurrent access to joining_
    unique_lock< spinlock > lk( joining_mtx_);
    BOOST_FOREACH( fiber_base::ptr_t & p, joining_)
    { p->set_ready(); }
}

bool
fiber_base::join( ptr_t const& p)
{
    // protect against concurrent access to joining_
    unique_lock< spinlock > lk( joining_mtx_);
    if ( is_terminated() ) return false;
    joining_.push_back( p);
    return true;
}

void
fiber_base::rethrow() const
{
    BOOST_ASSERT( has_exception() );

    rethrow_exception( except_);
}


void
fiber_base::set_ready()
{
    // this fiber calls set_ready(): - only transition from state_waiting (wake-up)
    //                               - or transition from state_running (yield) allowed
    // other fiber calls set_ready(): - only if this fiber was joinig other fiber
    //                                - if this fiber was not interrupted then this fiber
    //                                  should in state_waiting
    //                                - if this fiber was interrupted the this fiber might
    //                                  be in state_ready, state_running or already in
    //                                  state_terminated
    for (;;)
    {
        state_t expected = state_waiting;
        bool result = state_.compare_exchange_strong( expected, state_ready, memory_order_release);
        if ( result || state_terminated == expected || state_ready == expected) return;
        expected = state_running;
        result = state_.compare_exchange_strong( expected, state_ready, memory_order_release);
        if ( result || state_terminated == expected || state_ready == expected) return;
    }
}

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
