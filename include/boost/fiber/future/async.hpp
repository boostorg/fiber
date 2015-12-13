
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_ASYNC_HPP
#define BOOST_FIBERS_ASYNC_HPP

#include <algorithm>
#include <memory>
#include <type_traits>
#include <utility>

#include <boost/config.hpp>

#include <boost/fiber/future/future.hpp>
#include <boost/fiber/future/packaged_task.hpp>

namespace boost {
namespace fibers {

template< typename Fn, typename ... Args >
future< typename std::result_of< Fn( Args && ... ) >::type >
async( Fn && fn, Args && ... args) {
    typedef typename std::result_of< Fn( Args && ... ) >::type result_type;

    packaged_task< result_type( Args && ... ) > pt{ std::forward< Fn >( fn) };
    future< result_type > f{ pt.get_future() };
    fiber{ std::move( pt), std::forward< Args >( args) ... }.detach();
    return f;
}

template< typename StackAllocator, typename Fn, typename ... Args >
future< typename std::result_of< Fn( Args && ... ) >::type >
async( std::allocator_arg_t, StackAllocator salloc, Fn && fn, Args && ... args) {
    typedef typename std::result_of< Fn( Args && ... ) >::type result_type;

    packaged_task< result_type( Args && ... ) > pt{ std::forward< Fn >( fn) };
    future< result_type > f{ pt.get_future() };
    fiber{ salloc, std::move( pt), std::forward< Args >( args) ... }.detach();
    return f;
}

}}

#endif // BOOST_FIBERS_ASYNC_HPP
