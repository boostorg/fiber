
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_DETAIL_FIBER_OBJECT_H
#define BOOST_FIBERS_DETAIL_FIBER_OBJECT_H

#include <cstddef>

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/context/fcontext.hpp>
#include <boost/move/move.hpp>
#include <boost/ref.hpp>

#include <boost/fiber/attributes.hpp>
#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/fiber_base.hpp>
#include <boost/fiber/detail/flags.hpp>
#include <boost/fiber/flags.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

# if defined(BOOST_MSVC)
# pragma warning(push)
# pragma warning(disable:4355)
# endif

namespace boost {
namespace fibers {
namespace detail {

template< typename Fn, typename StackAllocator, typename Allocator >
class fiber_object : public fiber_base
{
public:
    typedef typename Allocator::template rebind<
        fiber_object<
            Fn, StackAllocator, Allocator
        >
    >::other                                            allocator_t;

private:
    typedef fiber_base                                  base_type;

    Fn                  fn_;
    allocator_t         alloc_;

    static void destroy_( allocator_t & alloc, fiber_object * p)
    {
        alloc.destroy( p);
        alloc.deallocate( p, 1);
    }

    fiber_object( fiber_object &);
    fiber_object & operator=( fiber_object const&);

public:
#ifndef BOOST_NO_RVALUE_REFERENCES
    fiber_object( Fn && fn, attributes const& attr,
                  StackAllocator const& stack_alloc,
                  allocator_t const& alloc) :
        base_type( attr, stack_alloc, alloc),
        fn_( forward< Fn >( fn) ),
        alloc_( alloc)
    {}
#else
    fiber_object( Fn fn, attributes const& attr,
                  StackAllocator const& stack_alloc,
                  allocator_t const& alloc) :
        base_type( attr, stack_alloc, alloc),
        fn_( fn),
        alloc_( alloc)
    {}

    fiber_object( BOOST_RV_REF( Fn) fn, attributes const& attr,
                  StackAllocator const& stack_alloc,
                  allocator_t const& alloc) :
        base_type( attr, stack_alloc, alloc),
        fn_( fn),
        alloc_( alloc)
    {}
#endif

    void deallocate_object()
    { destroy_( alloc_, this); }

    void run()
    { fn_(); }
};

}}}

# if defined(BOOST_MSVC)
# pragma warning(pop)
# endif

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DETAIL_FIBER_OBJECT_H
