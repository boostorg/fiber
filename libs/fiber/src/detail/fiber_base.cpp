
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

void
fiber_base::unwind_stack_()
{
    BOOST_ASSERT( ! is_complete() );
    BOOST_ASSERT( ! is_resumed() );

    flags_ |= flag_resumed;
    flags_ |= flag_unwind_stack;
    ctx::jump_fcontext( & caller_, & callee_, 0, preserve_fpu_);
    BOOST_ASSERT( is_complete() );
    BOOST_ASSERT( ! is_resumed() );
}

void
fiber_base::terminate_()
{
    BOOST_ASSERT( ! is_resumed() );

    if ( ! is_complete() )
    {
        flags_ |= flag_canceled;
        unwind_stack_();
    }

    notify_();

    BOOST_ASSERT( is_complete() );
    BOOST_ASSERT( ! is_resumed() );
    BOOST_ASSERT( joining_.empty() );
}

void
fiber_base::notify_()
{
    BOOST_ASSERT( is_complete() );
    BOOST_ASSERT( ! is_resumed() );

    BOOST_FOREACH( fiber_base::ptr_t & p, joining_)
    { scheduler::instance().notify( p); }
    joining_.clear();
}

fiber_base::fiber_base( std::size_t size, bool preserve_fpu) :
    use_count_( 0),
    alloc_(),
    caller_(),
    callee_(),
    flags_( 0),
    preserve_fpu_( preserve_fpu),
    joining_()
{
    callee_.fc_stack.base = alloc_.allocate( size);
    callee_.fc_stack.size = size;

    ctx::make_fcontext( & callee_, trampoline< fiber_base >);
}

fiber_base::~fiber_base()
{
    terminate_();
    alloc_.deallocate( callee_.fc_stack.base, callee_.fc_stack.size);
}

fiber_base::id
fiber_base::get_id() const
{ return id( ptr_t( const_cast< fiber_base * >( this) ) ); }

bool
fiber_base::is_canceled() const
{ return 0 != ( flags_ & flag_canceled); }

bool
fiber_base::is_complete() const
{ return 0 != ( flags_ & flag_complete); }

bool
fiber_base::is_resumed() const
{ return 0 != ( flags_ & flag_resumed); }

void
fiber_base::join( ptr_t const& p)
{
    BOOST_ASSERT( ! p->is_complete() );
    BOOST_ASSERT( p->is_resumed() );

    joining_.push_back( p);
    scheduler::instance().wait( p);
}

void
fiber_base::resume()
{
    BOOST_ASSERT( ! is_complete() );
    BOOST_ASSERT( ! is_resumed() );

    flags_ |= flag_resumed;
    ctx::jump_fcontext( & caller_, & callee_, ( intptr_t) this, preserve_fpu_);

    if ( is_complete() ) notify_();

    BOOST_ASSERT( ! is_resumed() );
}

void
fiber_base::suspend()
{
    BOOST_ASSERT( ! is_complete() );
    BOOST_ASSERT( is_resumed() );

    flags_ &= ~flag_resumed;
    ctx::jump_fcontext( & callee_, & caller_, 0, preserve_fpu_);

    BOOST_ASSERT( is_resumed() );

    if ( 0 != ( flags_ & flag_unwind_stack) )
        throw forced_unwind();
}

void
fiber_base::cancel()
{
    BOOST_ASSERT( ! is_complete() );
    BOOST_ASSERT( ! is_resumed() );

    scheduler::instance().cancel( this);
}

void
fiber_base::notify()
{
    BOOST_ASSERT( ! is_complete() );
    BOOST_ASSERT( ! is_resumed() );

    scheduler::instance().notify( this);
}

void
fiber_base::wait()
{
    BOOST_ASSERT( ! is_complete() );
    BOOST_ASSERT( is_resumed() );

    scheduler::instance().wait( this);

    BOOST_ASSERT( ! is_complete() );
    BOOST_ASSERT( is_resumed() );
}

void
fiber_base::sleep( chrono::system_clock::time_point const& abs_time)
{
    BOOST_ASSERT( ! is_complete() );
    BOOST_ASSERT( is_resumed() );

    scheduler::instance().sleep( abs_time);

    BOOST_ASSERT( ! is_complete() );
    BOOST_ASSERT( is_resumed() );
}

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
