
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_ASYNC_HPP
#define BOOST_FIBERS_ASYNC_HPP

#include <boost/config.hpp>
#include <boost/detail/scoped_enum_emulation.hpp>
#include <boost/move/move.hpp>
#include <boost/utility/result_of.hpp>

#include <boost/fiber/future/future.hpp>
#include <boost/fiber/future/packaged_task.hpp>

namespace boost {
namespace fibers {

#ifndef BOOST_NO_RVALUE_REFERENCES
#ifdef BOOST_MSVC
template< typename R >
future< R >
async( R( *f)() )
{
    packaged_task< R > pt( f);
    future< R > fi( pt.get_future() );
    fiber( move( pt) ).detach();
    return move( fi);
}
#endif
template< typename F >
future< typename result_of< F() >::type >
async( F && f)
{
    typedef typename result_of< F() >::type R;
    packaged_task< R() > pt( forward< F >( f) );
    future< R > fi( pt.get_future() );
    fiber( move( pt) ).detach();
    return move( fi);
}
#else
template< typename F >
future< typename result_of< F() >::type >
async( F f)
{
    typedef typename result_of< F() >::type R;
    packaged_task< R() > pt( forward< F >( f) );
    future< R > fi( pt.get_future() );
    fiber( move( pt) ).detach();
    return move( fi);
}

template< typename F >
future< typename result_of< F() >::type >
async( BOOST_RV_REF( F) f)
{
    typedef typename result_of< F() >::type R;
    packaged_task< R() > pt( forward< F >( f) );
    future< R > fi( pt.get_future() );
    fiber( move( pt) ).detach();
    return move( fi);
}
#endif

}}

#endif // BOOST_FIBERS_ASYNC_HPP
