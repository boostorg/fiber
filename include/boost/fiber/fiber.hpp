
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_FIBER_H
#define BOOST_FIBERS_FIBER_H

#include <algorithm>
#include <cstddef>
#include <exception> // std::terminate()
#include <memory>

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/move/move.hpp>
#include <boost/thread/detail/memory.hpp> // boost::allocator_arg_t
#include <boost/utility.hpp>
#include <boost/utility/explicit_operator_bool.hpp>

#include <boost/fiber/attributes.hpp>
#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/fiber_base.hpp>
#include <boost/fiber/detail/worker_fiber.hpp>
#include <boost/fiber/stack_allocator.hpp>
#include <boost/fiber/stack_context.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

# if defined(BOOST_MSVC)
# pragma warning(push)
# pragma warning(disable:4251 4275)
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

    typedef detail::fiber_base          base_t;
    typedef intrusive_ptr< base_t >     ptr_t;

    ptr_t                               impl_;

    BOOST_MOVABLE_BUT_NOT_COPYABLE( fiber)

#ifdef BOOST_MSVC
    typedef void ( * fiber_fn)();

    template< typename StackAllocator >
    base_t * make_fiber_( StackAllocator salloc, attributes const& attrs, fiber_fn fn)
    {
        typedef detail::worker_fiber< fiber_fn, StackAllocator >    wrk_t;

        // create a stack-context
        stack_context sctx;
        // allocate fiber-stack
        salloc.allocate( sctx, attrs.size);
        BOOST_ASSERT( 0 < sctx.sp);
        // reserve space for worker_fiber on top of stack
        std::size_t size = sctx.size - sizeof( wrk_t);
        void * sp = static_cast< char * >( sctx.sp) - sizeof( wrk_t);
        // placement new for worker_fiber
        return new( sp) wrk_t( boost::forward< fiber_fn >( fn), sp, size, salloc, sctx);
    }
#endif

#ifdef BOOST_NO_CXX11_RVALUE_REFERENCES
    template< typename StackAllocator, typename Fn >
    base_t * make_fiber_( StackAllocator salloc, attributes const& attrs, Fn fn)
    {
        typedef detail::worker_fiber< Fn, StackAllocator >    wrk_t;

        // create a stack-context
        stack_context sctx;
        // allocate fiber-stack
        salloc.allocate( sctx, attrs.size);
        BOOST_ASSERT( 0 < sctx.sp);
        // reserve space for worker_fiber on top of stack
        std::size_t size = sctx.size - sizeof( wrk_t);
        void * sp = static_cast< char * >( sctx.sp) - sizeof( wrk_t);
        // placement new for worker_fiber
        return new( sp) wrk_t( boost::forward< Fn >( fn), sp, size, salloc, sctx);
    }
#endif

    template< typename StackAllocator, typename Fn >
    base_t * make_fiber_( StackAllocator salloc, attributes const& attrs, BOOST_RV_REF( Fn) fn)
    {
        typedef detail::worker_fiber< Fn, StackAllocator >    wrk_t;

        // create a stack-context
        stack_context sctx;
        // allocate fiber-stack
        salloc.allocate( sctx, attrs.size);
        BOOST_ASSERT( 0 < sctx.sp);
        // reserve space for worker_fiber on top of stack
        std::size_t size = sctx.size - sizeof( wrk_t);
        void * sp = static_cast< char * >( sctx.sp) - sizeof( wrk_t);
        // placement new for worker_fiber
#ifdef BOOST_NO_RVALUE_REFERENCES
        return new( sp) wrk_t( fn, sp, size, salloc, sctx);
#else
        return new( sp) wrk_t( boost::forward< Fn >( fn), sp, size, salloc, sctx);
#endif
    }

    void start_();

public:
    typedef detail::fiber_base::id    id;

    fiber() BOOST_NOEXCEPT :
        impl_()
    {}

    explicit fiber( detail::fiber_base * impl) BOOST_NOEXCEPT :
        impl_( impl)
    {}


    template< typename Fn >
    fiber( Fn && fn) :
        impl_()
    {
        attributes attrs;
        stack_allocator salloc;
#ifdef BOOST_NO_RVALUE_REFERENCES
        impl_.reset( make_fiber_( salloc, attrs, fn) );
#else
        impl_.reset( make_fiber_( salloc, attrs, boost::forward< Fn >( fn) ) );
#endif
        start_();
    }

    template< typename Fn >
    fiber( attributes const& attrs, BOOST_RV_REF( Fn) fn) :
        impl_()
    {
        stack_allocator salloc;
#ifdef BOOST_NO_RVALUE_REFERENCES
        impl_.reset( make_fiber_( salloc, attrs, fn) );
#else
        impl_.reset( make_fiber_( salloc, attrs, boost::forward< Fn >( fn) ) );
#endif
        start_();
    }

    template< typename StackAllocator, typename Fn >
    fiber( boost::allocator_arg_t, StackAllocator salloc, BOOST_RV_REF( Fn) fn) :
        impl_()
    {
        attributes attrs;
#ifdef BOOST_NO_RVALUE_REFERENCES
        impl_.reset( make_fiber_( salloc, attrs, fn) );
#else
        impl_.reset( make_fiber_( salloc, attrs, boost::forward< Fn >( fn) ) );
#endif
        start_();
    }

    template< typename StackAllocator, typename Fn >
    fiber( boost::allocator_arg_t, StackAllocator salloc, attributes const& attrs, BOOST_RV_REF( Fn) fn) :
        impl_()
    {
#ifdef BOOST_NO_RVALUE_REFERENCES
        impl_.reset( make_fiber_( salloc, attrs, fn) );
#else
        impl_.reset( make_fiber_( salloc, attrs, boost::forward< Fn >( fn) ) );
#endif
        start_();
    }

#ifdef BOOST_FIBERS_USE_VARIADIC_FIBER
    template< typename Fn, class ... Args >
    fiber( BOOST_RV_REF( Fn) fn, BOOST_RV_REF( Args) ... args) :
        impl_()
    {
        attributes attrs;
        stack_allocator salloc;
        impl_.reset(
            make_fiber_( salloc, attrs, std::bind( std::forward< Fn >( fn), std::forward< Args >( args) ... ) ) );
        start_();
    }

    template< typename Fn, class ... Args >
    fiber( attributes const& attrs, BOOST_RV_REF( Fn) fn, BOOST_RV_REF( Args) ... args) :
        impl_()
    {
        stack_allocator salloc;
        impl_.reset(
            make_fiber_( salloc, attrs, std::bind( std::forward< Fn >( fn), std::forward< Args >( args) ... ) ) );
        start_();
    }

    template< typename StackAllocator, typename Fn, class ... Args >
    fiber( boost::allocator_arg_t, StackAllocator salloc, BOOST_RV_REF( Fn) fn, BOOST_RV_REF( Args) ... args) :
        impl_()
    {
        attributes attrs;
        impl_.reset(
            make_fiber_( salloc, attrs, std::bind( std::forward< Fn >( fn), std::forward< Args >( args) ... ) ) );
        start_();
    }

    template< typename StackAllocator, typename Fn, class ... Args >
    fiber( boost::allocator_arg_t, StackAllocator salloc, attributes const& attrs,
           BOOST_RV_REF( Fn) fn, BOOST_RV_REF( Args) ... args) :
        impl_()
    {
        impl_.reset(
            make_fiber_( salloc, attrs, std::bind( std::forward< Fn >( fn), std::forward< Args >( args) ... ) ) );
        start_();
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
        fiber tmp( boost::move( other) );
        swap( tmp);
        return * this;
    }

    BOOST_EXPLICIT_OPERATOR_BOOL();

    bool operator!() const BOOST_NOEXCEPT
    { return 0 == impl_ || impl_->is_terminated(); }

    void swap( fiber & other) BOOST_NOEXCEPT
    { impl_.swap( other.impl_); }

    bool joinable() const BOOST_NOEXCEPT
    { return 0 != impl_ /* && ! impl_->is_terminated() */; }

    id get_id() const BOOST_NOEXCEPT
    { return 0 != impl_ ? impl_->get_id() : id(); }

    int priority() const BOOST_NOEXCEPT;

    void priority( int) BOOST_NOEXCEPT;

    bool thread_affinity() const BOOST_NOEXCEPT;

    void thread_affinity( bool) BOOST_NOEXCEPT;

    void detach() BOOST_NOEXCEPT;

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
