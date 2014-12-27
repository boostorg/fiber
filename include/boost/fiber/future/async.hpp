
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_ASYNC_HPP
#define BOOST_FIBERS_ASYNC_HPP

#include <algorithm> // std::move()
#include <type_traits> // std::result_of
#include <utility> // std::forward()

#include <boost/config.hpp>

#include <boost/fiber/future/future.hpp>
#include <boost/fiber/future/packaged_task.hpp>

namespace boost {
namespace fibers {

template< typename Fn >
future< typename std::result_of< Fn() >::type >
async( Fn fn) {
    typedef typename std::result_of< Fn() >::type result_type;

    packaged_task< result_type() > pt( std::forward< Fn >( fn) );
    future< result_type > f( pt.get_future() );
    fiber( std::move( pt) ).detach();
    return std::move( f);
}

}}

#endif // BOOST_FIBERS_ASYNC_HPP
