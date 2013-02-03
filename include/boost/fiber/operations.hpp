//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_THIS_FIBER_OPERATIONS_H
#define BOOST_THIS_FIBER_OPERATIONS_H

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/move/move.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/arithmetic/inc.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/repetition/repeat_from_to.hpp>

#include <boost/fiber/detail/fiber_base.hpp>
#include <boost/fiber/detail/scheduler.hpp>
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
void yield()
{
    BOOST_ASSERT( is_fiberized() );
    fibers::detail::scheduler::instance().yield();
}

}

namespace fibers {

inline
algorithm * scheduling_algorithm( algorithm * al)
{ return detail::scheduler::replace( al); }

#if 0
#define BOOST_FIBERS_WAITFOR_FIBER_FN_ARG(z,n,unused) \
    fiber & BOOST_PP_CAT(f,n)

#define BOOST_FIBERS_WAITFOR_FIBER_FN_ARGS(n) \
	BOOST_PP_ENUM(n,BOOST_FIBERS_WAITFOR_FIBER_FN_ARG,~)

#define BOOST_FIBERS_WAITFOR_FIBER_AND(z,n,t) \
	BOOST_PP_EXPR_IF(n,&&) BOOST_PP_CAT(f,n)

#define BOOST_FIBERS_WAITFOR_FIBER_OR(z,n,t) \
	BOOST_PP_EXPR_IF(n,||) BOOST_PP_CAT(f,n)

#define BOOST_FIBERS_WAITFOR_FIBER_CANCEL(z,n,t) \
	if ( BOOST_PP_CAT(f,n) ) BOOST_PP_CAT(f,n).cancel(); \
    else i = BOOST_PP_INC(n);

#define BOOST_FIBERS_WAITFOR_FIBER_READY(z,n,t) \
	if ( ! BOOST_PP_CAT(f,n) ) return BOOST_PP_INC(n);

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
    BOOST_PP_REPEAT(n,BOOST_FIBERS_WAITFOR_FIBER_READY,~); \
    return 0; \
}

#define BOOST_FIBERS_WAITFOR_FIBER_ANY_AND_CANCEL(z,n,unused) \
inline \
unsigned int waitfor_any_and_cancel( BOOST_FIBERS_WAITFOR_FIBER_FN_ARGS(n) ) \
{ \
	while ( BOOST_PP_REPEAT(n,BOOST_FIBERS_WAITFOR_FIBER_AND,~) ) \
		run(); \
    unsigned int i = 0; \
    BOOST_PP_REPEAT(n,BOOST_FIBERS_WAITFOR_FIBER_CANCEL,~); \
    return i; \
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
#endif
}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_THIS_FIBER_OPERATIONS_H
