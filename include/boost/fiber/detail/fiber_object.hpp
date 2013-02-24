
//          Copyright Oliver Kowalke 2009.
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

namespace boost {
namespace fibers {
namespace detail {

struct forced_unwind {};

template< typename Fiber >
void trampoline( intptr_t vp)
{
    BOOST_ASSERT( vp);

    Fiber * f( reinterpret_cast< Fiber * >( vp) );
    BOOST_ASSERT( f->is_running() );
    f->exec();
}

template< typename Fn, typename StackAllocator >
class fiber_object : public fiber_base
{
private:
    Fn                  fn_;
    context::stack_t    stack_;
    StackAllocator      stack_alloc_;

    fiber_object( fiber_object &);
    fiber_object & operator=( fiber_object const&);

    void enter_()
    {
        set_running();
        context::jump_fcontext(
            & caller_, callee_,
            reinterpret_cast< intptr_t >( this),
            preserve_fpu() );
        BOOST_ASSERT( ! except_);
    }

protected:
    void unwind_stack() BOOST_NOEXCEPT
    {
        flags_ |= flag_unwind_stack;
        set_running();
        context::jump_fcontext(
            & caller_, callee_,
            0, preserve_fpu() );
        flags_ &= ~flag_unwind_stack;

        BOOST_ASSERT( is_terminated() );
    }

public:
#ifndef BOOST_NO_RVALUE_REFERENCES
    fiber_object( BOOST_RV_REF( Fn) fn, attributes const& attr,
                  StackAllocator const& stack_alloc) :
        fiber_base(
            context::make_fcontext(
                stack_alloc.allocate( attr.size), attr.size,
                trampoline< fiber_object >),
            fpu_preserved == attr.preserve_fpu),
        fn_( forward< Fn >( fn) ),
        stack_( fiber_base::callee_->fc_stack),
        stack_alloc_( stack_alloc)
    { enter_(); }
#else
    fiber_object( Fn fn, attributes const& attr,
                  StackAllocator const& stack_alloc) :
        fiber_base(
            context::make_fcontext(
                stack_alloc.allocate( attr.size), attr.size,
                trampoline< fiber_object >),
            fpu_preserved == attr.preserve_fpu),
        fn_( fn),
        stack_( fiber_base::callee_->fc_stack),
        stack_alloc_( stack_alloc)
    { enter_(); }

    fiber_object( BOOST_RV_REF( Fn) fn, attributes const& attr,
                  StackAllocator const& stack_alloc) :
        fiber_base(
            context::make_fcontext(
                stack_alloc.allocate( attr.size), attr.size,
                trampoline< fiber_object >),
            fpu_preserved == attr.preserve_fpu),
        fn_( fn),
        stack_( fiber_base::callee_->fc_stack),
        stack_alloc_( stack_alloc)
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
        context::jump_fcontext( callee_, & caller_, 0, preserve_fpu() );
        BOOST_ASSERT_MSG( false, "fiber already terminated");
    }
};

template< typename Fn, typename StackAllocator >
class fiber_object< reference_wrapper< Fn >, StackAllocator > : public fiber_base
{
private:
    Fn                  fn_;
    context::stack_t    stack_;
    StackAllocator      stack_alloc_;

    fiber_object( fiber_object &);
    fiber_object & operator=( fiber_object const&);

    void enter_()
    {
        set_running();
        context::jump_fcontext(
            & caller_, callee_,
            reinterpret_cast< intptr_t >( this),
            preserve_fpu() );
        BOOST_ASSERT( ! except_);
    }

protected:
    void unwind_stack() BOOST_NOEXCEPT
    {
        flags_ |= flag_unwind_stack;
        set_running();
        context::jump_fcontext(
            & caller_, callee_,
            0, preserve_fpu() );
        flags_ &= ~flag_unwind_stack;

        BOOST_ASSERT( is_terminated() );
    }

public:
    fiber_object( reference_wrapper< Fn > fn, attributes const& attr,
                  StackAllocator const& stack_alloc) :
        fiber_base(
            context::make_fcontext(
                stack_alloc.allocate( attr.size), attr.size,
                trampoline< fiber_object >),
            fpu_preserved == attr.preserve_fpu),
        fn_( fn),
        stack_( fiber_base::callee_->fc_stack),
        stack_alloc_( stack_alloc)
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
        context::jump_fcontext( callee_, & caller_, 0, preserve_fpu() );
        BOOST_ASSERT_MSG( false, "fiber already terminated");
    }
};

template< typename Fn, typename StackAllocator >
class fiber_object< const reference_wrapper< Fn >, StackAllocator > : public fiber_base
{
private:
    Fn                  fn_;
    context::stack_t    stack_;
    StackAllocator      stack_alloc_;

    fiber_object( fiber_object &);
    fiber_object & operator=( fiber_object const&);

    void enter_()
    {
        set_running();
        context::jump_fcontext(
            & caller_, callee_,
            reinterpret_cast< intptr_t >( this),
            preserve_fpu() );
        BOOST_ASSERT( ! except_);
    }

protected:
    void unwind_stack() BOOST_NOEXCEPT
    {
        flags_ |= flag_unwind_stack;
        set_running();
        context::jump_fcontext(
            & caller_, callee_,
            0, preserve_fpu() );
        flags_ &= ~flag_unwind_stack;

        BOOST_ASSERT( is_terminated() );
    }

public:
    fiber_object( const reference_wrapper< Fn > fn, attributes const& attr,
                  StackAllocator const& stack_alloc) :
        fiber_base(
            context::make_fcontext(
                stack_alloc.allocate( attr.size), attr.size,
                trampoline< fiber_object >),
            fpu_preserved == attr.preserve_fpu),
        fn_( fn),
        stack_( fiber_base::callee_->fc_stack),
        stack_alloc_( stack_alloc)
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
        context::jump_fcontext( callee_, & caller_, 0, preserve_fpu() );
        BOOST_ASSERT_MSG( false, "fiber already terminated");
    }
};

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DETAIL_FIBER_OBJECT_H
