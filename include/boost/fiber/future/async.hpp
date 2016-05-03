
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
#include <boost/fiber/policy.hpp>

namespace boost {
namespace fibers {

template< class Function, class ... Args >
future<
    typename std::result_of<
        typename std::decay< Function >::type( typename std::decay< Args >::type ... )
    >::type
>
async( launch_policy lpol, Function && fn, Args && ... args) {
    typedef typename std::result_of<
        typename std::decay< Function >::type( typename std::decay< Args >::type ... )
    >::type     result_t;

    packaged_task< result_t( typename std::decay< Args >::type ... ) > pt{
        std::forward< Function >( fn) };
    future< result_t > f{ pt.get_future() };
    fiber{ lpol, std::move( pt), std::forward< Args >( args) ... }.detach();
    return f;
}

template< class Function, class ... Args >
future<
    typename std::result_of<
        typename std::decay< Function >::type( typename std::decay< Args >::type ... )
    >::type
>
async( Function && fn, Args && ... args) {
    typedef typename std::result_of<
        typename std::decay< Function >::type( typename std::decay< Args >::type ... )
    >::type     result_t;

    packaged_task< result_t( typename std::decay< Args >::type ... ) > pt{
        std::forward< Function >( fn) };
    future< result_t > f{ pt.get_future() };
    fiber{ std::move( pt), std::forward< Args >( args) ... }.detach();
    return f;
}

template< typename StackAllocator, class Function, class ... Args >
future<
    typename std::result_of<
        typename std::decay< Function >::type( typename std::decay< Args >::type ... )
    >::type
>
async( launch_policy lpol, std::allocator_arg_t, StackAllocator salloc, Function && fn, Args && ... args) {
    typedef typename std::result_of<
        typename std::decay< Function >::type( typename std::decay< Args >::type ... )
    >::type     result_t;

    packaged_task< result_t( typename std::decay< Args >::type ... ) > pt{
        std::allocator_arg, salloc, std::forward< Function >( fn) };
    future< result_t > f{ pt.get_future() };
    fiber{ lpol, std::move( pt), std::forward< Args >( args) ... }.detach();
    return f;
}

template< typename StackAllocator, class Function, class ... Args >
future<
    typename std::result_of<
        typename std::decay< Function >::type( typename std::decay< Args >::type ... )
    >::type
>
async( std::allocator_arg_t, StackAllocator salloc, Function && fn, Args && ... args) {
    typedef typename std::result_of<
        typename std::decay< Function >::type( typename std::decay< Args >::type ... )
    >::type     result_t;

    packaged_task< result_t( typename std::decay< Args >::type ... ) > pt{
        std::allocator_arg, salloc, std::forward< Function >( fn) };
    future< result_t > f{ pt.get_future() };
    fiber{ std::move( pt), std::forward< Args >( args) ... }.detach();
    return f;
}

}}

#endif // BOOST_FIBERS_ASYNC_HPP
