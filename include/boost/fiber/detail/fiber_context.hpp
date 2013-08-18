
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_DETAIL_FIBER_CONTEXT_H
#define BOOST_FIBERS_DETAIL_FIBER_CONTEXT_H

#include <cstddef>

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/context/fcontext.hpp>

#include "boost/fiber/detail/config.hpp"
#include "boost/fiber/stack_context.hpp"

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

# if defined(BOOST_MSVC)
# pragma warning(push)
# pragma warning(disable:4275)
# endif

#if defined(BOOST_USE_SEGMENTED_STACKS)
extern "C"  void *__splitstack_makecontext(
        std::size_t, void * [BOOST_FIBERS_SEGMENTS], std::size_t *);
#endif

namespace boost {
namespace fibers {
namespace detail {

class BOOST_FIBERS_DECL fiber_context : private context::fcontext_t,
                                        private stack_context
                    
{
private:
    stack_context       *   stack_ctx_;
    context::fcontext_t *   ctx_;

public:
    typedef void( * ctx_fn)( intptr_t);

    fiber_context();

    explicit fiber_context( ctx_fn, stack_context *);

    fiber_context( fiber_context const&);

    fiber_context & operator=( fiber_context const&);

    intptr_t jump( fiber_context &, intptr_t = 0, bool = true);
};

}}}

# if defined(BOOST_MSVC)
# pragma warning(pop)
# endif

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DETAIL_FIBER_CONTEXT_H
