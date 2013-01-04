
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_FIBERS_SOURCE

#include <boost/fiber/detail/fiber_base.hpp>

#include <boost/foreach.hpp>

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
    priority_( 0),
    caller_(),
    callee_( callee),
    flags_( 0),
    except_(),
    mtx_(),
    joining_()
{ if ( preserve_fpu) flags_ |= flag_preserve_fpu; }

void
fiber_base::resume()
{
    BOOST_ASSERT( is_running() );

    context::jump_fcontext( & caller_, callee_, 0, preserve_fpu() );

    BOOST_ASSERT( ! is_running() );
}

void
fiber_base::suspend()
{
    BOOST_ASSERT( is_waiting() );

    context::jump_fcontext( callee_, & caller_, 0, preserve_fpu() );

    BOOST_ASSERT( is_running() );

    if ( unwind_requested() )
        throw forced_unwind();
}

void
fiber_base::yield()
{
    BOOST_ASSERT( is_ready() );

    context::jump_fcontext( callee_, & caller_, 0, preserve_fpu() );

//    BOOST_ASSERT( is_running() );

    if ( unwind_requested() )
        throw forced_unwind();
}

void
fiber_base::terminate()
{
    if ( ! is_terminated() ) unwind_stack();

    // fiber_base::terminate() is called by ~fiber_object()
    // therefore protecting by mtx_ is not required
    // and joining_ is not required to be cleared
    BOOST_FOREACH( fiber_base::ptr_t & p, joining_)
    { p->set_ready(); }
}

void
fiber_base::join( ptr_t const& p)
{
    BOOST_ASSERT( p->is_running() );

    // protect against concurrent access to joining_
    spin_mutex::scoped_lock lk( mtx_);
    if ( is_terminated() ) return;
    joining_.push_back( p);
}

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
