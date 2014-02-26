
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_DETAIL_FIBER_OBJECT_H
#define BOOST_FIBERS_DETAIL_FIBER_OBJECT_H

#include <cstddef>

#include <boost/assert.hpp>
#include <boost/bind.hpp>
#include <boost/config.hpp>
#include <boost/coroutine/all.hpp>
#include <boost/exception_ptr.hpp>
#include <boost/move/move.hpp>
#include <boost/ref.hpp>

#include <boost/fiber/attributes.hpp>
#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/fiber_base.hpp>
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

namespace coro = boost::coroutines;

template< typename Fn, typename StackAllocator, typename Allocator >
class fiber_object : public fiber_base
{
public:
    typedef typename Allocator::template rebind<
        fiber_object< Fn, StackAllocator, Allocator >
    >::other                                allocator_t;

private:
    typedef fiber_base                      base_t;
    typedef coro::symmetric_coroutine<
        void, StackAllocator
    >                                       coro_t;

    Fn                              fn_;
    typename coro_t::yield_type *   callee_;
    typename coro_t::call_type      caller_;
    allocator_t                     alloc_;

    static void destroy_( allocator_t & alloc, fiber_object * p)
    {
        alloc.destroy( p);
        alloc.deallocate( p, 1);
    }

    fiber_object( fiber_object &);
    fiber_object & operator=( fiber_object const&);

    void trampoline_( typename coro_t::yield_type & yield)
    {
        BOOST_ASSERT( yield);
        BOOST_ASSERT( ! is_terminated() );

        callee_ = & yield;
        set_running();
        suspend();

        try
        {
            BOOST_ASSERT( is_running() );
            fn_();
            BOOST_ASSERT( is_running() );
        }
        catch ( coro::detail::forced_unwind const&)
        {
            set_terminated();
            release();
            throw;
        }
        catch ( fiber_interrupted const&)
        { except_ = current_exception(); }
        catch (...)
        { std::terminate(); }

        set_terminated();
        release();
        suspend();

        BOOST_ASSERT_MSG( false, "fiber already terminated");
    }

public:
#ifndef BOOST_NO_RVALUE_REFERENCES
    fiber_object( Fn && fn, attributes const& attrs,
                  StackAllocator const& stack_alloc,
                  allocator_t const& alloc) :
        base_t(),
        fn_( forward< Fn >( fn) ),
        callee_( 0),
        caller_(
            boost::bind( & fiber_object::trampoline_, this, _1),
            attrs,
            stack_alloc),
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
    fiber_object( Fn fn, attributes const& attrs,
                  StackAllocator const& stack_alloc,
                  allocator_t const& alloc) :
        base_t(),
        fn_( fn),
        callee_( 0),
        caller_(
            boost::bind( & fiber_object::trampoline_, this, _1),
            attrs,
            stack_alloc),
        alloc_( alloc)
    {
        BOOST_ASSERT( caller_);
        BOOST_ASSERT( 0 == callee_);

        caller_(); // jump to trampoline

        BOOST_ASSERT( 0 != callee_);
        BOOST_ASSERT( * callee_);

        set_ready(); // fiber is setup and now ready to run
    }

    fiber_object( BOOST_RV_REF( Fn) fn, attributes const& attrs,
                  StackAllocator const& stack_alloc,
                  allocator_t const& alloc) :
        base_t(),
        fn_( fn),
        callee_( 0),
        caller_(
            boost::bind( & fiber_object::trampoline_, this, _1),
            attrs,
            stack_alloc),
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

    void resume()
    {
        BOOST_ASSERT( caller_);
        BOOST_ASSERT( is_running() ); // set by the scheduler-algorithm

        caller_();
    }

    void suspend()
    {
        BOOST_ASSERT( callee_);
        BOOST_ASSERT( * callee_);

        ( * callee_)();

        BOOST_ASSERT( is_running() ); // set by the scheduler-algorithm
    }
};

}}}

# if defined(BOOST_MSVC)
# pragma warning(pop)
# endif

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DETAIL_FIBER_OBJECT_H
