
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
    f->exec();
}

template< typename Fn, typename StackAllocator, typename Allocator >
class fiber_object : public fiber_base
{
public:
    typedef typename Allocator::template rebind<
        fiber_object<
            Fn, StackAllocator, Allocator
        >
    >::other                                        allocator_t;

private:
    Fn                  fn_;
    context::stack_t    stack_;
    StackAllocator      stack_alloc_;
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
        context::jump_fcontext(
            & this->caller_, this->callee_,
            reinterpret_cast< intptr_t >( this),
            this->preserve_fpu() );
        if ( this->except_) rethrow_exception( this->except_);
    }

    void unwind_stack_()
    {
    
    }

public:
#ifndef BOOST_NO_RVALUE_REFERENCES
    fiber_object( BOOST_RV_REF( Fn) fn, attributes const& attr,
                  StackAllocator const& stack_alloc,
                  allocator_t const& alloc) :
        fiber_base(
            context::make_fcontext(
                stack_alloc.allocate( attr.size), attr.size,
                trampoline< fiber_object >),
            stack_unwind == attr.do_unwind,
            fpu_preserved == attr.preserve_fpu),
        fn_( forward< Fn >( fn) ),
        stack_( fiber_base::callee_->fc_stack),
        stack_alloc_( stack_alloc),
        alloc_( alloc)
    { enter_(); }
#else
    fiber_object( Fn fn, attributes const& attr,
                  StackAllocator const& stack_alloc,
                  allocator_t const& alloc) :
        fiber_base(
            context::make_fcontext(
                stack_alloc.allocate( attr.size), attr.size,
                trampoline< fiber_object >),
            stack_unwind == attr.do_unwind,
            fpu_preserved == attr.preserve_fpu),
        fn_( fn),
        stack_( fiber_base::callee_->fc_stack),
        stack_alloc_( stack_alloc),
        alloc_( alloc)
    { enter_(); }

    fiber_object( BOOST_RV_REF( Fn) fn, attributes const& attr,
                  StackAllocator const& stack_alloc,
                  allocator_t const& alloc) :
        fiber_base(
            context::make_fcontext(
                stack_alloc.allocate( attr.size), attr.size,
                trampoline< fiber_object >),
            stack_unwind == attr.do_unwind,
            fpu_preserved == attr.preserve_fpu),
        fn_( fn),
        stack_( fiber_base::callee_->fc_stack),
        stack_alloc_( stack_alloc),
        alloc_( alloc)
    { enter_(); }
#endif

    void exec()
    {
        BOOST_ASSERT( ! is_complete() );

        suspend();

        try
        { fn_(); }
        catch ( forced_unwind const&)
        {}
        catch (...)
        { except_ = current_exception(); }

        flags_ &= ~flag_resumed;
        flags_ |= flag_complete;

        context::jump_fcontext( callee_, & caller_, 0, preserve_fpu() );
        BOOST_ASSERT_MSG( false, "fiber is complete");
    }

    void deallocate_object()
    { destroy_( alloc_, this); }

    void terminate()
    {
        BOOST_ASSERT( ! is_resumed() );

        if ( ! is_complete() )
        {
            flags_ |= flag_canceled;
            unwind_stack_();
        }

        notify_();

        BOOST_ASSERT( is_complete() );
        BOOST_ASSERT( ! is_resumed() );
        BOOST_ASSERT( joining_.empty() );
    }
};

template< typename Fn, typename StackAllocator, typename Allocator >
class fiber_object< reference_wrapper< Fn >, StackAllocator, Allocator > : public fiber_base
{
public:
    typedef typename Allocator::template rebind<
        fiber_object<
            Fn, StackAllocator, Allocator
        >
    >::other                                        allocator_t;

private:
    Fn                  fn_;
    context::stack_t    stack_;
    StackAllocator      stack_alloc_;
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
        context::jump_fcontext(
            & this->caller_, this->callee_,
            reinterpret_cast< intptr_t >( this),
            this->preserve_fpu() );
        if ( this->except_) rethrow_exception( this->except_);
    }

public:
    fiber_object( reference_wrapper< Fn > fn, attributes const& attr,
                  StackAllocator const& stack_alloc,
                  allocator_t const& alloc) :
        fiber_base(
            context::make_fcontext(
                stack_alloc.allocate( attr.size), attr.size,
                trampoline< fiber_object >),
            stack_unwind == attr.do_unwind,
            fpu_preserved == attr.preserve_fpu),
        fn_( fn),
        stack_( fiber_base::callee_->fc_stack),
        stack_alloc_( stack_alloc),
        alloc_( alloc)
    { enter_(); }

    void exec()
    {
        BOOST_ASSERT( ! is_complete() );

        suspend();

        try
        { fn_(); }
        catch ( forced_unwind const&)
        {}
        catch (...)
        { except_ = current_exception(); }

        flags_ &= ~flag_resumed;
        flags_ |= flag_complete;

        context::jump_fcontext( callee_, & caller_, 0, preserve_fpu() );
        BOOST_ASSERT_MSG( false, "fiber is complete");
    }

    void deallocate_object()
    { destroy_( alloc_, this); }
};

template< typename Fn, typename StackAllocator, typename Allocator >
class fiber_object< const reference_wrapper< Fn >, StackAllocator, Allocator > : public fiber_base
{
public:
    typedef typename Allocator::template rebind<
        fiber_object<
            Fn, StackAllocator, Allocator
        >
    >::other                                        allocator_t;

private:
    Fn                  fn_;
    context::stack_t    stack_;
    StackAllocator      stack_alloc_;
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
        context::jump_fcontext(
            & this->caller_, this->callee_,
            reinterpret_cast< intptr_t >( this),
            this->preserve_fpu() );
        if ( this->except_) rethrow_exception( this->except_);
    }

public:
    fiber_object( const reference_wrapper< Fn > fn, attributes const& attr,
                  StackAllocator const& stack_alloc,
                  allocator_t const& alloc) :
        fiber_base(
            context::make_fcontext(
                stack_alloc.allocate( attr.size), attr.size,
                trampoline< fiber_object >),
            stack_unwind == attr.do_unwind,
            fpu_preserved == attr.preserve_fpu),
        fn_( fn),
        stack_( fiber_base::callee_->fc_stack),
        stack_alloc_( stack_alloc),
        alloc_( alloc)
    { enter_(); }

    void exec()
    {
        BOOST_ASSERT( ! is_complete() );

        suspend();

        try
        { fn_(); }
        catch ( forced_unwind const&)
        {}
        catch (...)
        { except_ = current_exception(); }

        flags_ &= ~flag_resumed;
        flags_ |= flag_complete;

        context::jump_fcontext( callee_, & caller_, 0, preserve_fpu() );
        BOOST_ASSERT_MSG( false, "fiber is complete");
    }

    void deallocate_object()
    { destroy_( alloc_, this); }
};

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DETAIL_FIBER_OBJECT_H
