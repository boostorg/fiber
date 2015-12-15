
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
namespace detail {

// based on libstdc++-v3
template< typename Fn, typename ... Args >
using result_of = 
    typename std::result_of<
        typename std::decay< Fn >::type( typename std::decay< Args >::type ... )
    >::type;

}

template< typename Fn, typename ... Args >
future< detail::result_of< Fn, Args ... > >
async( Fn && fn, Args && ... args) {
    using result_t = detail::result_of< Fn, Args ... >;

    packaged_task< result_t( Args ... ) > pt{ std::forward< Fn >( fn) };
    future< result_t > f{ pt.get_future() };
    fiber{ std::move( pt), std::forward< Args >( args) ... }.detach();
    return f;
}

template< typename StackAllocator, typename Fn, typename ... Args >
future< detail::result_of< Fn, Args ... > >
async( std::allocator_arg_t, StackAllocator salloc, Fn && fn, Args && ... args) {
    using result_t = detail::result_of< Fn, Args ... >;

    packaged_task< result_t( Args ... ) > pt{
        std::allocator_arg, salloc, std::forward< Fn >( fn) };
    future< result_t > f{ pt.get_future() };
    fiber{ std::move( pt), std::forward< Args >( args) ... }.detach();
    return f;
}

}}

#endif // BOOST_FIBERS_ASYNC_HPP
