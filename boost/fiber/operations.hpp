//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_THIS_FIBER_OPERATIONS_H
#define BOOST_THIS_FIBER_OPERATIONS_H

#include <boost/assert.hpp>
#include <boost/chrono/system_clocks.hpp>
#include <boost/config.hpp>
#include <boost/move/move.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/repetition/repeat_from_to.hpp>

#include <boost/fiber/detail/scheduler.hpp>
#include <boost/fiber/detail/fiber_base.hpp>
#include <boost/fiber/fiber.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace this_fiber {

inline
bool is_fiberized()
{ return fibers::detail::scheduler::instance().active(); }

inline
fibers::fiber::id get_id()
{
	BOOST_ASSERT( is_fiberized() );
	return fibers::detail::scheduler::instance().active()->get_id();
}

inline
void sleep( chrono::system_clock::time_point const& abs_time)
{
	BOOST_ASSERT( is_fiberized() );
	fibers::detail::scheduler::instance().sleep( abs_time);
}

template< typename TimeDuration > 
void sleep( TimeDuration const& dt)
{ sleep( chrono::system_clock::now() + dt); }

inline
void yield()
{
	BOOST_ASSERT( is_fiberized() );
	fibers::detail::scheduler::instance().yield();
}

inline
void yield_break()
{
	BOOST_ASSERT( is_fiberized() );
	throw fibers::detail::forced_unwind();
}

}

namespace fibers {

#ifndef BOOST_NO_RVALUE_REFERENCES
#ifdef BOOST_MSVC
#endif
#else
template< typename Fn >
fiber spawn( Fn fn, attributes const& attr = attributes(),
             stack_allocator const& stack_alloc = stack_allocator(),
             std::allocator< fiber > const& alloc = std::allocator< fiber >() )
{
    fiber f( fn, attr, stack_alloc, alloc);
    detail::scheduler::instance().spawn( f.impl_);
    return f;
}

template< typename Fn, typename StackAllocator >
fiber spawn( Fn fn, attributes const& attr,
             StackAllocator const& stack_alloc,
             std::allocator< fiber > const& alloc = std::allocator< fiber >() )
{
    fiber f( fn, attr, stack_alloc, alloc);
    detail::scheduler::instance().spawn( f.impl_);
    return f;
}

template< typename Fn, typename StackAllocator, typename Allocator >
fiber spawn( Fn fn, attributes const& attr,
             StackAllocator const& stack_alloc,
             Allocator const& alloc)
{
    fiber f( fn, attr, stack_alloc, alloc);
    detail::scheduler::instance().spawn( f.impl_);
    return f;
}

template< typename Fn >
fiber spawn( BOOST_RV_REF( Fn) fn, attributes const& attr = attributes(),
             stack_allocator const& stack_alloc = stack_allocator(),
             std::allocator< fiber > const& alloc = std::allocator< fiber >() )
{
    fiber f( fn, attr, stack_alloc, alloc);
    detail::scheduler::instance().spawn( f.impl_);
    return f;
}

template< typename Fn, typename StackAllocator >
fiber spawn( BOOST_RV_REF( Fn) fn, attributes const& attr,
             StackAllocator const& stack_alloc,
             std::allocator< fiber > const& alloc = std::allocator< fiber >() )
{
    fiber f( fn, attr, stack_alloc, alloc);
    detail::scheduler::instance().spawn( f.impl_);
    return f;
}

template< typename Fn, typename StackAllocator, typename Allocator >
fiber spawn( BOOST_RV_REF( Fn) fn, attributes const& attr,
             StackAllocator const& stack_alloc,
             Allocator const& alloc)
{
    fiber f( fn, attr, stack_alloc, alloc);
    detail::scheduler::instance().spawn( f.impl_);
    return f;
}
#endif

inline
bool run()
{ return detail::scheduler::instance().run(); }

#define BOOST_FIBERS_WAITFOR_FIBER_FN_ARG(z,n,unused) \
    fiber & BOOST_PP_CAT(s,n)

#define BOOST_FIBERS_WAITFOR_FIBER_FN_ARGS(n) \
	BOOST_PP_ENUM(n,BOOST_FIBERS_WAITFOR_FIBER_FN_ARG,~)

#define BOOST_FIBERS_WAITFOR_FIBER_AND(z,n,t) \
	BOOST_PP_EXPR_IF(n,&&) BOOST_PP_CAT(s,n)

#define BOOST_FIBERS_WAITFOR_FIBER_OR(z,n,t) \
	BOOST_PP_EXPR_IF(n,||) BOOST_PP_CAT(s,n)

#define BOOST_FIBERS_WAITFOR_FIBER_CANCEL(z,n,t) \
	if ( BOOST_PP_CAT(s,n) ) BOOST_PP_CAT(s,n).cancel();

#define BOOST_FIBERS_WAITFOR_FIBER_READY(z,n,t) \
	if ( ! BOOST_PP_CAT(s,n) ) return n;

#define BOOST_FIBERS_WAITFOR_FIBER_ALL(z,n,unused) \
inline \
void waitfor_all( BOOST_FIBERS_WAITFOR_FIBER_FN_ARGS(n) ) \
{ \
	while ( BOOST_PP_REPEAT(n,BOOST_FIBERS_WAITFOR_FIBER_OR,~) ) \
		run(); \
}

#define BOOST_FIBERS_WAITFOR_FIBER_ANY(z,n,unused) \
inline \
unsigned int waitfor_any( BOOST_FIBERS_WAITFOR_FIBER_FN_ARGS(n) ) \
{ \
	while ( BOOST_PP_REPEAT(n,BOOST_FIBERS_WAITFOR_FIBER_AND,~) ) \
		run(); \
    return 0; \
}

#define BOOST_FIBERS_WAITFOR_FIBER_ANY_AND_CANCEL(z,n,unused) \
inline \
unsigned int waitfor_any_and_cancel( BOOST_FIBERS_WAITFOR_FIBER_FN_ARGS(n) ) \
{ \
	while ( BOOST_PP_REPEAT(n,BOOST_FIBERS_WAITFOR_FIBER_AND,~) ) \
		run(); \
    BOOST_PP_REPEAT(n,BOOST_FIBERS_WAITFOR_FIBER_CANCEL,~); \
    return 0; \
}

#ifndef BOOST_FIBERS_WAITFOR_FIBER_MAX_ARITY
#define BOOST_FIBERS_WAITFOR_FIBER_MAX_ARITY 12
#endif

BOOST_PP_REPEAT_FROM_TO( 2, BOOST_FIBERS_WAITFOR_FIBER_MAX_ARITY, BOOST_FIBERS_WAITFOR_FIBER_ALL, ~)
BOOST_PP_REPEAT_FROM_TO( 2, BOOST_FIBERS_WAITFOR_FIBER_MAX_ARITY, BOOST_FIBERS_WAITFOR_FIBER_ANY, ~)
BOOST_PP_REPEAT_FROM_TO( 2, BOOST_FIBERS_WAITFOR_FIBER_MAX_ARITY, BOOST_FIBERS_WAITFOR_FIBER_ANY_AND_CANCEL, ~)

#undef BOOST_FIBERS_WAITFOR_FIBER_READY
#undef BOOST_FIBERS_WAITFOR_FIBER_ALL
#undef BOOST_FIBERS_WAITFOR_FIBER_ANY
#undef BOOST_FIBERS_WAITFOR_FIBER_ANY_AND_CANCEL
#undef BOOST_FIBERS_WAITFOR_FIBER_CANCEL
#undef BOOST_FIBERS_WAITFOR_FIBER_OR
#undef BOOST_FIBERS_WAITFOR_FIBER_AND
#undef BOOST_FIBERS_WAITFOR_FIBER_ARGS
#undef BOOST_FIBERS_WAITFOR_FIBER_ARG

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_THIS_FIBER_OPERATIONS_H
