
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_DETAIL_STACK_TUPLE_H
#define BOOST_FIBERS_DETAIL_STACK_TUPLE_H

#include <cstddef>

#include <boost/config.hpp>

#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/stack_context.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace detail {

template< typename StackAllocator >
struct stack_tuple
{
    fibers::stack_context       stack_ctx;
    StackAllocator              stack_alloc;

    stack_tuple( StackAllocator const& stack_alloc_, std::size_t size) :
        stack_ctx(),
        stack_alloc( stack_alloc_)
    { stack_alloc.allocate( stack_ctx, size); }

    ~stack_tuple()
    { stack_alloc.deallocate( stack_ctx); }
};


}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DETAIL_STACK_TUPLE_H