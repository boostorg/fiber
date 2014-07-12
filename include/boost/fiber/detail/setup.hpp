
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_DETAIL_SETUP_H
#define BOOST_FIBERS_DETAIL_SETUP_H

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/move/move.hpp>
#include <boost/type_traits/decay.hpp>
#include <boost/type_traits/is_convertible.hpp>
#include <boost/type_traits/is_same.hpp>

#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/worker_fiber.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace detail {

template< typename Fn >
struct setup
{
    typedef typename worker_fiber::coro_t   coro_t;

    struct dummy {};

    Fn                              fn;
    typename coro_t::call_type  *   caller;
    worker_fiber                *   f;

#ifdef BOOST_NO_CXX11_RVALUE_REFERENCES
    setup( Fn fn_,
           coro_t::call_type * caller_) :
        fn( forward< Fn >( fn_) ),
        caller( caller_),
        f( 0)
    { BOOST_ASSERT( 0 != caller); }
#endif

    setup( BOOST_RV_REF( Fn) fn_,
           coro_t::call_type * caller_,
           typename disable_if<
               is_same< typename decay< Fn >::type, setup >,
               dummy*
           >::type = 0) :
#ifdef BOOST_NO_CXX11_RVALUE_REFERENCES
        fn( fn_),
#else
        fn( forward< Fn >( fn_) ),
#endif
        caller( caller_),
        f( 0)
    { BOOST_ASSERT( 0 != caller); }

    worker_fiber * allocate()
    {
        // enter fiber-fn (trampoline<>)
        // and pas this as argument
        ( * caller)( this);
        // jumped back; move coroutine to worker_fiber
        f->caller_ = boost::move( * caller);
        return f;        
    }
};

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DETAIL_SETUP_H
