
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBER_PREALLOCATED_STACK_ALLOCATOR_H
#define BOOST_FIBER_PREALLOCATED_STACK_ALLOCATOR_H

#include <cstddef>
#include <cstdlib>
#include <stdexcept>
#include <vector>

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/context/stack_context.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

class preallocated_stack_allocator
{
private:
    typedef std::vector< boost::context::stack_context > cont_t;
    cont_t      stacks_;
    std::size_t idx_;

public:
    preallocated_stack_allocator( std::size_t size = 1) :
        stacks_(), idx_( 0)
    {
        boost::context::fixedsize allocator;
        for ( std::size_t i = 0; i < size; ++i)
        {
            stacks_.push_back( allocator.allocate() );
        }
    }

    boost::context::stack_context allocate()
    {
        boost::context::stack_context ctx;
        ctx.sp = stacks_[idx_].sp;
        ctx.size = stacks_[idx_].size;
        ++idx_;
        return ctx;
    }

    void deallocate( boost::context::stack_context & ctx)
    {
    }
};

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBER_PREALLOCATED_STACK_ALLOCATOR_H
