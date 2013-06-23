
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_FIBER_H
#define BOOST_FIBERS_FIBER_H

#include <cstddef>
#include <exception>
#include <memory>

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/move/move.hpp>
#include <boost/type_traits/decay.hpp>
#include <boost/type_traits/is_convertible.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/utility.hpp>

#include <boost/fiber/attributes.hpp>
#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/fiber_base.hpp>
#include <boost/fiber/detail/fiber_object.hpp>
#include <boost/fiber/stack_allocator.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

# if defined(BOOST_MSVC)
# pragma warning(push)
# pragma warning(disable:4251)
# endif

namespace boost {
namespace fibers {
namespace detail {

class scheduler;

}

class BOOST_FIBERS_DECL fiber : private noncopyable
{
private:
    friend class detail::scheduler;

    typedef detail::fiber_base    base_t;
    typedef base_t::ptr_t         ptr_t;

    struct dummy
    { void nonnull() {} };

    typedef void ( dummy::*safe_bool)();

    ptr_t       impl_;

    BOOST_MOVABLE_BUT_NOT_COPYABLE( fiber);

    void start_fiber_();

public:
    typedef detail::fiber_base::id        id;

    fiber() BOOST_NOEXCEPT :
        impl_()
    {}

    fiber( ptr_t const& imp) BOOST_NOEXCEPT :
        impl_( imp)
    {}

#ifndef BOOST_NO_RVALUE_REFERENCES
#ifdef BOOST_MSVC
    typedef void ( * fiber_fn)();

    explicit fiber( fiber_fn fn, attributes const& attr = attributes(),
                    stack_allocator const& stack_alloc = stack_allocator(),
                    std::allocator< push_coroutine > const& alloc =
                        std::allocator< push_coroutine >(),
                    typename disable_if<
                        is_same< typename decay< fiber_fn >::type, fiber >,
                        dummy *
                    >::type = 0) :
        impl_()
    {
        typedef detail::fiber_object<
                fiber_fn, stack_allocator, std::allocator< fiber >
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( forward< fiber_fn >( fn), attr, stack_alloc, a) );
        start_fiber_();
    }

    template< typename StackAllocator >
    explicit fiber( fiber_fn fn, attributes const& attr,
                    StackAllocator const& stack_alloc,
                    std::allocator< push_coroutine > const& alloc =
                        std::allocator< push_coroutine >(),
                    typename disable_if<
                        is_same< typename decay< fiber_fn >::type, fiber >,
                        dummy *
                    >::type = 0) :
        impl_()
    {
        typedef detail::fiber_object<
                fiber_fn, StackAllocator, std::allocator< fiber >
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( forward< fiber_fn >( fn), attr, stack_alloc, a) );
        start_fiber_();
    }

    template< typename StackAllocator, typename Allocator >
    explicit fiber( fiber_fn fn, attributes const& attr,
                    StackAllocator const& stack_alloc,
                    Allocator const& alloc,
                    typename disable_if<
                        is_same< typename decay< fiber_fn >::type, fiber >,
                        dummy *
                    >::type = 0) :
        impl_()
    {
        typedef detail::fiber_object<
                fiber_fn, StackAllocator, Allocator
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( forward< fiber_fn >( fn), attr, stack_alloc, a) );
        start_fiber_();
    }
#endif
    template< typename Fn >
    explicit fiber( BOOST_RV_REF( Fn) fn, attributes const& attr = attributes(),
                    stack_allocator const& stack_alloc = stack_allocator(),
                    std::allocator< fiber > const& alloc =
                         std::allocator< fiber >(),
                    typename disable_if<
                        is_same< typename decay< Fn >::type, fiber >,
                        dummy *
                    >::type = 0) :
        impl_()
    {
        typedef detail::fiber_object<
                Fn, stack_allocator, std::allocator< fiber >
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( forward< Fn >( fn), attr, stack_alloc, a) );
        start_fiber_();
    }

    template< typename Fn, typename StackAllocator >
    explicit fiber( BOOST_RV_REF( Fn) fn, attributes const& attr,
                    StackAllocator const& stack_alloc,
                    std::allocator< fiber > const& alloc =
                         std::allocator< fiber >(),
                    typename disable_if<
                        is_same< typename decay< Fn >::type, fiber >,
                        dummy *
                    >::type = 0) :
        impl_()
    {
        typedef detail::fiber_object<
                Fn, StackAllocator, std::allocator< fiber >
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( forward< Fn >( fn), attr, stack_alloc, a) );
        start_fiber_();
    }

    template< typename Fn, typename StackAllocator, typename Allocator >
    explicit fiber( BOOST_RV_REF( Fn) fn, attributes const& attr,
                    StackAllocator const& stack_alloc,
                    Allocator const& alloc,
                    typename disable_if<
                        is_same< typename decay< Fn >::type, fiber >,
                        dummy *
                    >::type = 0) :
        impl_()
    {
        typedef detail::fiber_object<
                Fn, StackAllocator, Allocator
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( forward< Fn >( fn), attr, stack_alloc, a) );
        start_fiber_();
    }
#else
    template< typename Fn >
    explicit fiber( Fn fn, attributes const& attr = attributes(),
                    stack_allocator const& stack_alloc = stack_allocator(),
                    std::allocator< fiber > const& alloc =
                         std::allocator< fiber >(),
                    typename disable_if<
                        is_convertible< Fn &, BOOST_RV_REF( Fn) >,
                        dummy *
                    >::type = 0) :
        impl_()
    {
        typedef detail::fiber_object<
                Fn, stack_allocator, std::allocator< fiber >
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( fn, attr, stack_alloc, a) );
        start_fiber_();
    }

    template< typename Fn, typename StackAllocator >
    explicit fiber( Fn fn, attributes const& attr,
                    StackAllocator const& stack_alloc,
                    std::allocator< fiber > const& alloc =
                         std::allocator< fiber >(),
                    typename disable_if<
                        is_convertible< Fn &, BOOST_RV_REF( Fn) >,
                        dummy *
                    >::type = 0) :
        impl_()
    {
        typedef detail::fiber_object<
                Fn, StackAllocator, std::allocator< fiber >
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( fn, attr, stack_alloc, a) );
        start_fiber_();
    }

    template< typename Fn, typename StackAllocator, typename Allocator >
    explicit fiber( Fn fn, attributes const& attr,
                    StackAllocator const& stack_alloc,
                    Allocator const& alloc,
                    typename disable_if<
                        is_convertible< Fn &, BOOST_RV_REF( Fn) >,
                        dummy *
                    >::type = 0) :
        impl_()
    {
        typedef detail::fiber_object<
                Fn, StackAllocator, Allocator
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( fn, attr, stack_alloc, a) );
        start_fiber_();
    }

    template< typename Fn >
    explicit fiber( BOOST_RV_REF( Fn) fn, attributes const& attr = attributes(),
                    stack_allocator const& stack_alloc = stack_allocator(),
                    std::allocator< fiber > const& alloc =
                         std::allocator< fiber >(),
                    typename disable_if<
                        is_same< typename decay< Fn >::type, fiber >,
                        dummy *
                    >::type = 0) :
        impl_()
    {
        typedef detail::fiber_object<
                Fn, stack_allocator, std::allocator< fiber >
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( fn, attr, stack_alloc, a) );
        start_fiber_();
    }

    template< typename Fn, typename StackAllocator >
    explicit fiber( BOOST_RV_REF( Fn) fn, attributes const& attr,
                    StackAllocator const& stack_alloc,
                    std::allocator< fiber > const& alloc =
                         std::allocator< fiber >(),
                    typename disable_if<
                        is_same< typename decay< Fn >::type, fiber >,
                        dummy *
                    >::type = 0) :
        impl_()
    {
        typedef detail::fiber_object<
                Fn, StackAllocator, std::allocator< fiber >
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( fn, attr, stack_alloc, a) );
        start_fiber_();
    }

    template< typename Fn, typename StackAllocator, typename Allocator >
    explicit fiber( BOOST_RV_REF( Fn) fn, attributes const& attr,
                    StackAllocator const& stack_alloc,
                    Allocator const& alloc,
                    typename disable_if<
                        is_same< typename decay< Fn >::type, fiber >,
                        dummy *
                    >::type = 0) :
        impl_()
    {
        typedef detail::fiber_object<
                Fn, StackAllocator, Allocator
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( fn, attr, stack_alloc, a) );
        start_fiber_();
    }
#endif

    ~fiber()
    { if ( joinable() ) std::terminate(); }

    fiber( BOOST_RV_REF( fiber) other) BOOST_NOEXCEPT :
        impl_()
    { swap( other); }

    fiber & operator=( BOOST_RV_REF( fiber) other) BOOST_NOEXCEPT
    {
        if ( joinable() ) std::terminate();
        fiber tmp( move( other) );
        swap( tmp);
        return * this;
    }

    operator safe_bool() const BOOST_NOEXCEPT
    { return impl_ && ! impl_->is_terminated() ? & dummy::nonnull : 0; }

    bool operator!() const BOOST_NOEXCEPT
    { return ! impl_ || impl_->is_terminated(); }

    void swap( fiber & other) BOOST_NOEXCEPT
    { impl_.swap( other.impl_); }

    bool joinable() const BOOST_NOEXCEPT
    { return 0 != impl_.get() /* && ! impl_->is_terminated() */; }

    id get_id() const BOOST_NOEXCEPT
    { return impl_ ? impl_->get_id() : id(); }

    int priority() const BOOST_NOEXCEPT;

    void priority( int) BOOST_NOEXCEPT;

    void detach() BOOST_NOEXCEPT
    { impl_.reset(); }

    void join();

    void interrupt() BOOST_NOEXCEPT;
};

inline
bool operator<( fiber const& l, fiber const& r)
{ return l.get_id() < r.get_id(); }

inline
void swap( fiber & l, fiber & r)
{ return l.swap( r); }

}}

# if defined(BOOST_MSVC)
# pragma warning(pop)
# endif

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_FIBER_H
