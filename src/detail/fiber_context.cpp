
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_FIBERS_SOURCE

#include "boost/fiber/detail/fiber_context.hpp"

#ifdef BOOST_MSVC
 #pragma warning (push)
 #pragma warning (disable: 4355) // using 'this' in initializer list
#endif

#if defined(BOOST_USE_SEGMENTED_STACKS)
extern "C" {

void __splitstack_getcontext( void * [BOOST_FIBERS_SEGMENTS]);

void __splitstack_setcontext( void * [BOOST_FIBERS_SEGMENTS]);

void __splitstack_releasecontext (void * [BOOST_FIBERS_SEGMENTS]);

void __splitstack_block_signals_context( void * [BOOST_FIBERS_SEGMENTS], int *, int *);

}
#endif

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace detail {

fiber_context::fiber_context() :
    fcontext_t(), stack_ctx_( this), ctx_( this)
{
#if defined(BOOST_USE_SEGMENTED_STACKS)
    __splitstack_getcontext( stack_ctx_->segments_ctx);
#endif
}

fiber_context::fiber_context( ctx_fn fn, stack_context * stack_ctx) :
    fcontext_t(), stack_ctx_( stack_ctx),
    ctx_( context::make_fcontext( stack_ctx_->sp, stack_ctx_->size, fn) )
{}

fiber_context::fiber_context( fiber_context const& other) :
    fcontext_t(),
    stack_ctx_( other.stack_ctx_),
    ctx_( other.ctx_)
{}

fiber_context &
fiber_context::operator=( fiber_context const& other)
{
    if ( this == & other) return * this;

    stack_ctx_ = other.stack_ctx_;
    ctx_ = other.ctx_;

    return * this;
}

intptr_t
fiber_context::jump( fiber_context & other, intptr_t param, bool preserve_fpu)
{
#if defined(BOOST_USE_SEGMENTED_STACKS)
    BOOST_ASSERT( stack_ctx_);
    BOOST_ASSERT( other.stack_ctx_);

    __splitstack_getcontext( stack_ctx_->segments_ctx);
    __splitstack_setcontext( other.stack_ctx_->segments_ctx);
    intptr_t ret = context::jump_fcontext( ctx_, other.ctx_, param, preserve_fpu);

    BOOST_ASSERT( stack_ctx_);
    __splitstack_setcontext( stack_ctx_->segments_ctx);

    return ret;
#else
    return context::jump_fcontext( ctx_, other.ctx_, param, preserve_fpu);
#endif
}

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#ifdef BOOST_MSVC
 #pragma warning (pop)
#endif
