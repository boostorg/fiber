
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
#include <boost/fiber/detail/stack_tuple.hpp>
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

template< typename Fiber >
void trampoline( intptr_t vp)
{
    BOOST_ASSERT( vp);

    Fiber * f( reinterpret_cast< Fiber * >( vp) );
    BOOST_ASSERT( f->is_running() );
    f->exec();
}

template< typename Fn, typename StackAllocator, typename Allocator >
class fiber_object : private stack_tuple< StackAllocator >,
                     public fiber_base
{
public:
    typedef typename Allocator::template rebind<
        fiber_object<
            Fn, StackAllocator, Allocator
        >
    >::other                                            allocator_t;

private:
    typedef stack_tuple< StackAllocator >               pbase_type;
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

    void enter_()
    {
        set_running();
        caller_.jump(
            callee_,
            reinterpret_cast< intptr_t >( this),
            preserve_fpu() );
        BOOST_ASSERT( ! except_);
    }

protected:
    void unwind_stack() BOOST_NOEXCEPT
    {
        flags_ |= flag_unwind_stack;
        set_running();
        caller_.jump(
            callee_,
            0,
            preserve_fpu() );
        flags_ &= ~flag_unwind_stack;
        BOOST_ASSERT( is_terminated() );
    }

public:
#ifndef BOOST_NO_RVALUE_REFERENCES
    fiber_object( Fn && fn, attributes const& attr,
                  StackAllocator const& stack_alloc,
                  allocator_t const& alloc) :
        pbase_type( stack_alloc, attr.size),
        base_type(
            trampoline< fiber_object >,
            & this->stack_ctx,
            fpu_preserved == attr.preserve_fpu),
        fn_( forward< Fn >( fn) ),
        alloc_( alloc)
    { enter_(); }
#else
    fiber_object( Fn fn, attributes const& attr,
                  StackAllocator const& stack_alloc,
                  allocator_t const& alloc) :
        pbase_type( stack_alloc, attr.size),
        base_type(
            trampoline< fiber_object >,
            & this->stack_ctx,
            fpu_preserved == attr.preserve_fpu),
        fn_( fn),
        alloc_( alloc)
    { enter_(); }

    fiber_object( BOOST_RV_REF( Fn) fn, attributes const& attr,
                  StackAllocator const& stack_alloc,
                  allocator_t const& alloc) :
        pbase_type( stack_alloc, attr.size),
        base_type(
            trampoline< fiber_object >,
            & this->stack_ctx,
            fpu_preserved == attr.preserve_fpu),
        fn_( fn),
        alloc_( alloc)
    { enter_(); }
#endif

    ~fiber_object()
    {
        if ( ! is_terminated() )
            unwind_stack();
    }

    void exec()
    {
        BOOST_ASSERT( ! is_terminated() );

        try
        {
            set_ready();
            suspend();
            BOOST_ASSERT( is_running() );
            fn_();
            BOOST_ASSERT( is_running() );
        }
        catch ( forced_unwind const&)
        {}
        catch (...)
        { except_ = current_exception(); }

        set_terminated();
        release();
        callee_.jump(
            caller_,
            0,
            preserve_fpu() );
        BOOST_ASSERT_MSG( false, "fiber already terminated");
    }

    void deallocate_object()
    { destroy_( alloc_, this); }
};

template< typename Fn, typename StackAllocator, typename Allocator >
class fiber_object< reference_wrapper< Fn >, StackAllocator, Allocator > :
    private stack_tuple< StackAllocator >,
    public fiber_base
{
public:
    typedef typename Allocator::template rebind<
        fiber_object<
            Fn, StackAllocator, Allocator
        >
    >::other                                            allocator_t;

private:
    typedef stack_tuple< StackAllocator >               pbase_type;
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

    void enter_()
    {
        set_running();
        caller_.jump(
            callee_,
            reinterpret_cast< intptr_t >( this),
            preserve_fpu() );
        BOOST_ASSERT( ! except_);
    }

protected:
    void unwind_stack() BOOST_NOEXCEPT
    {
        flags_ |= flag_unwind_stack;
        set_running();
        caller_.jump(
            callee_,
            0,
            preserve_fpu() );
        flags_ &= ~flag_unwind_stack;
        BOOST_ASSERT( is_terminated() );
    }

public:
    fiber_object( reference_wrapper< Fn > fn, attributes const& attr,
                  StackAllocator const& stack_alloc,
                  allocator_t const& alloc) :
        pbase_type( stack_alloc, attr.size),
        base_type(
            trampoline< fiber_object >,
            & this->stack_ctx,
            fpu_preserved == attr.preserve_fpu),
        fn_( fn),
        alloc_( alloc)
    { enter_(); }

    ~fiber_object()
    {
        if ( ! is_terminated() )
            unwind_stack();
    }

    void exec()
    {
        BOOST_ASSERT( ! is_terminated() );

        try
        {
            set_ready();
            suspend();
            BOOST_ASSERT( is_running() );
            fn_();
            BOOST_ASSERT( is_running() );
        }
        catch ( forced_unwind const&)
        {}
        catch (...)
        { except_ = current_exception(); }

        set_terminated();
        release();
        callee_.jump(
            caller_,
            0,
            preserve_fpu() );
        BOOST_ASSERT_MSG( false, "fiber already terminated");
    }

    void deallocate_object()
    { destroy_( alloc_, this); }
};

template< typename Fn, typename StackAllocator, typename Allocator >
class fiber_object< const reference_wrapper< Fn >, StackAllocator, Allocator > :
    private stack_tuple< StackAllocator >,
    public fiber_base
{
public:
    typedef typename Allocator::template rebind<
        fiber_object<
            Fn, StackAllocator, Allocator
        >
    >::other                                            allocator_t;

private:
    typedef stack_tuple< StackAllocator >               pbase_type;
    typedef fiber_base                                  base_type;

    Fn                  fn_;
    allocator_t         alloc_;

    fiber_object( fiber_object &);
    fiber_object & operator=( fiber_object const&);

    static void destroy_( allocator_t & alloc, fiber_object * p)
    {
        alloc.destroy( p);
        alloc.deallocate( p, 1);
    }

    void enter_()
    {
        set_running();
        caller_.jump(
            callee_,
            reinterpret_cast< intptr_t >( this),
            preserve_fpu() );
        BOOST_ASSERT( ! except_);
    }

protected:
    void unwind_stack() BOOST_NOEXCEPT
    {
        flags_ |= flag_unwind_stack;
        set_running();
        caller_.jump(
            callee_,
            0,
            preserve_fpu() );
        flags_ &= ~flag_unwind_stack;
        BOOST_ASSERT( is_terminated() );
    }

public:
    fiber_object( const reference_wrapper< Fn > fn, attributes const& attr,
                  StackAllocator const& stack_alloc,
                  allocator_t const& alloc) :
        pbase_type( stack_alloc, attr.size),
        base_type(
            trampoline< fiber_object >,
            & this->stack_ctx,
            fpu_preserved == attr.preserve_fpu),
        fn_( forward< Fn >( fn) ),
        alloc_( alloc)
    { enter_(); }

    ~fiber_object()
    {
        if ( ! is_terminated() )
            unwind_stack();
    }

    void exec()
    {
        BOOST_ASSERT( ! is_terminated() );

        try
        {
            set_ready();
            suspend();
            fn_();
            BOOST_ASSERT( is_running() );
            fn_();
            BOOST_ASSERT( is_running() );
        }
        catch ( forced_unwind const&)
        {}
        catch (...)
        { except_ = current_exception(); }

        set_terminated();
        release();
        callee_.jump(
            caller_,
            0,
            preserve_fpu() );
        BOOST_ASSERT_MSG( false, "fiber already terminated");
    }

    void deallocate_object()
    { destroy_( alloc_, this); }
};

}}}

# if defined(BOOST_MSVC)
# pragma warning(pop)
# endif

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DETAIL_FIBER_OBJECT_H
