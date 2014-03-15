
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_DETAIL_WORKER_OBJECT_H
#define BOOST_FIBERS_DETAIL_WORKER_OBJECT_H

#include <cstddef>

#include <boost/assert.hpp>
#include <boost/bind.hpp>
#include <boost/config.hpp>
#include <boost/exception_ptr.hpp>
#include <boost/move/move.hpp>
#include <boost/ref.hpp>

#include <boost/fiber/attributes.hpp>
#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/worker_fiber.hpp>
#include <boost/fiber/exceptions.hpp>

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

template< typename Fn, typename Allocator >
class worker_object : public worker_fiber
{
public:
    typedef typename Allocator::template rebind<
        worker_object< Fn, Allocator >
    >::other                            allocator_t;

#ifndef BOOST_NO_RVALUE_REFERENCES
    worker_object( Fn && fn, attributes const& attrs,
                   allocator_t const& alloc) :
        base_t( attrs),
        fn_( forward< Fn >( fn) ),
        alloc_( alloc)
    {
        BOOST_ASSERT( caller_);
        BOOST_ASSERT( 0 == callee_);

        caller_(); // jump to trampoline

        BOOST_ASSERT( 0 != callee_);
        BOOST_ASSERT( * callee_);

        set_ready(); // fiber is setup and now ready to run
    }
#else
    worker_object( Fn fn, attributes const& attrs,
                   allocator_t const& alloc) :
        base_t( attrs),
        fn_( fn),
        alloc_( alloc)
    {
        BOOST_ASSERT( caller_);
        BOOST_ASSERT( 0 == callee_);

        caller_(); // jump to trampoline

        BOOST_ASSERT( 0 != callee_);
        BOOST_ASSERT( * callee_);

        set_ready(); // fiber is setup and now ready to run
    }

    worker_object( BOOST_RV_REF( Fn) fn, attributes const& attrs,
                   allocator_t const& alloc) :
        base_t( attrs),
        fn_( fn),
        alloc_( alloc)
    {
        BOOST_ASSERT( caller_);
        BOOST_ASSERT( 0 == callee_);

        caller_(); // jump to trampoline

        BOOST_ASSERT( 0 != callee_);
        BOOST_ASSERT( * callee_);

        set_ready(); // fiber is setup and now ready to run
    }
#endif

    void deallocate_object()
    { destroy_( alloc_, this); }

private:
    typedef worker_fiber                base_t;

    Fn              fn_;
    allocator_t     alloc_;

    static void destroy_( allocator_t & alloc, worker_object * p)
    {
        alloc.destroy( p);
        alloc.deallocate( p, 1);
    }

    worker_object( worker_object &);
    worker_object & operator=( worker_object const&);

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

#endif // BOOST_FIBERS_DETAIL_WORKER_OBJECT_H
