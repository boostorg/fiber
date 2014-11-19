
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_FIBER_H
#define BOOST_FIBERS_FIBER_H

#include <algorithm>
#include <cstddef>
#include <exception>
#include <memory>

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/coroutine/symmetric_coroutine.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/move/move.hpp>
#include <boost/thread/detail/memory.hpp> // boost::allocator_arg_t
#include <boost/utility.hpp>
#include <boost/utility/explicit_operator_bool.hpp>

#include <boost/fiber/attributes.hpp>
#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/setup.hpp>
#include <boost/fiber/detail/trampoline.hpp>
#include <boost/fiber/detail/worker_fiber.hpp>
#include <boost/fiber/fiber_manager.hpp>
#include <boost/fiber/stack_allocator.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

# if defined(BOOST_MSVC)
# pragma warning(push)
# pragma warning(disable:4251 4275)
# endif

namespace boost {
namespace fibers {

namespace coro = boost::coroutines;

namespace detail {

class scheduler;

}

class BOOST_FIBERS_DECL fiber : private noncopyable
{
private:
    friend class detail::scheduler;

    typedef detail::worker_fiber        base_t;
    typedef base_t::coro_t              coro_t;
    typedef intrusive_ptr< base_t >     ptr_t;

    ptr_t                               impl_;

    BOOST_MOVABLE_BUT_NOT_COPYABLE( fiber)

#ifdef BOOST_MSVC
    typedef void ( * fiber_fn)();

    template< typename StackAllocator >
    void setup_( StackAllocator const& stack_alloc, attributes const& attrs, fiber_fn fn)
    {
        coro_t::call_type coro( detail::trampoline< fiber_fn >, attrs, stack_alloc); 
        detail::setup< fiber_fn > s( boost::forward< fiber_fn >( fn), & coro);
        impl_.reset( s.allocate() );
        BOOST_ASSERT( impl_);
    }
#endif

#ifdef BOOST_NO_CXX11_RVALUE_REFERENCES
    template< typename StackAllocator, typename Fn >
    void setup_( StackAllocator const& stack_alloc, attributes const& attrs, Fn fn)
    {
        typename coro_t::call_type coro( detail::trampoline< Fn >, attrs, stack_alloc); 
        detail::setup< Fn > s( boost::forward< Fn >( fn), & coro);
        impl_.reset( s.allocate() );
        BOOST_ASSERT( impl_);
    }
#endif

    template< typename StackAllocator, typename Fn >
    void setup_( StackAllocator const& stack_alloc, attributes const& attrs, BOOST_RV_REF( Fn) fn)
    {
        typename coro_t::call_type coro( detail::trampoline< Fn >, attrs, stack_alloc); 
#ifdef BOOST_NO_RVALUE_REFERENCES
        detail::setup< Fn > s( fn, & coro);
#else
        detail::setup< Fn > s( boost::forward< Fn >( fn), & coro);
#endif
        impl_.reset( s.allocate() );
        BOOST_ASSERT( impl_);
    }

    void start_();

public:
    typedef detail::worker_fiber::id    id;

    fiber() BOOST_NOEXCEPT :
        impl_()
    {}

    // This fiber_base* is allowed to be 0 -- call joinable() before performing
    // operations on such a fiber object!
    explicit fiber( fiber_base * impl) BOOST_NOEXCEPT :
        impl_( dynamic_cast<detail::worker_fiber*>(impl))
    {}

#ifdef BOOST_MSVC
    fiber( fiber_fn fn) :
        impl_()
    {
        attributes attrs;
        stack_allocator stack_alloc;
        setup_( stack_alloc, attrs, fn);
        start_();
    }

    fiber( attributes const& attrs, fiber_fn fn) :
        impl_()
    {
        stack_allocator stack_alloc;
        setup_( stack_alloc, attrs, fn);
        start_();
    }

    template< typename StackAllocator >
    fiber( boost::allocator_arg_t, StackAllocator const& stack_alloc, fiber_fn fn) :
        impl_()
    {
        attributes attrs;
        setup_( stack_alloc, attrs, fn);
        start_();
    }

    template< typename StackAllocator >
    fiber( boost::allocator_arg_t, StackAllocator const& stack_alloc, attributes const& attrs, fiber_fn fn) :
        impl_()
    {
        setup_( stack_alloc, attrs, fn);
        start_();
    }
#endif

#ifdef BOOST_NO_RVALUE_REFERENCES
    template< typename Fn >
    fiber( Fn fn) :
        impl_()
    {
        attributes attrs;
        stack_allocator stack_alloc;
        setup_( stack_alloc, attrs, fn);
        start_();
    }

    template< typename Fn >
    fiber( attributes const& attrs, Fn fn) :
        impl_()
    {
        stack_allocator stack_alloc;
        setup_( stack_alloc, attrs, fn);
        start_();
    }

    template< typename StackAllocator, typename Fn >
    fiber( boost::allocator_arg_t, StackAllocator const& stack_alloc, Fn fn) :
        impl_()
    {
        attributes attrs;
        setup_( stack_alloc, attrs, fn);
        start_();
    }

    template< typename StackAllocator, typename Fn >
    fiber( boost::allocator_arg_t, StackAllocator const& stack_alloc, attributes const& attrs, Fn fn) :
        impl_()
    {
        setup_( stack_alloc, attrs, fn);
        start_();
    }
#endif

    template< typename Fn >
    fiber( BOOST_RV_REF( Fn) fn) :
        impl_()
    {
        attributes attrs;
        stack_allocator stack_alloc;
#ifdef BOOST_NO_RVALUE_REFERENCES
        setup_( stack_alloc, attrs, fn);
#else
        setup_( stack_alloc, attrs, boost::forward< Fn >( fn) );
#endif
        start_();
    }

    template< typename Fn >
    fiber( attributes const& attrs, BOOST_RV_REF( Fn) fn) :
        impl_()
    {
        stack_allocator stack_alloc;
#ifdef BOOST_NO_RVALUE_REFERENCES
        setup_( stack_alloc, attrs, fn);
#else
        setup_( stack_alloc, attrs, boost::forward< Fn >( fn) );
#endif
        start_();
    }

    template< typename StackAllocator, typename Fn >
    fiber( boost::allocator_arg_t, StackAllocator const& stack_alloc, BOOST_RV_REF( Fn) fn) :
        impl_()
    {
        attributes attrs;
#ifdef BOOST_NO_RVALUE_REFERENCES
        setup_( stack_alloc, attrs, fn);
#else
        setup_( stack_alloc, attrs, boost::forward< Fn >( fn) );
#endif
        start_();
    }

    template< typename StackAllocator, typename Fn >
    fiber( boost::allocator_arg_t, StackAllocator const& stack_alloc, attributes const& attrs, BOOST_RV_REF( Fn) fn) :
        impl_()
    {
#ifdef BOOST_NO_RVALUE_REFERENCES
        setup_( stack_alloc, attrs, fn);
#else
        setup_( stack_alloc, attrs, boost::forward< Fn >( fn) );
#endif
        start_();
    }

#ifdef BOOST_FIBERS_USE_VARIADIC_FIBER
    template< typename Fn, class ... Args >
    fiber( BOOST_RV_REF( Fn) fn, BOOST_RV_REF( Args) ... args) :
        impl_()
    {
        attributes attrs;
        stack_allocator stack_alloc;
        setup_( stack_alloc, attrs, std::bind( std::forward< Fn >( fn), std::forward< Args >( args) ... ) );
        start_();
    }

    template< typename Fn, class ... Args >
    fiber( attributes const& attrs, BOOST_RV_REF( Fn) fn, BOOST_RV_REF( Args) ... args) :
        impl_()
    {
        stack_allocator stack_alloc;
        setup_( stack_alloc, attrs, std::bind( std::forward< Fn >( fn), std::forward< Args >( args) ... ) );
        start_();
    }

    template< typename StackAllocator, typename Fn, class ... Args >
    fiber( boost::allocator_arg_t, StackAllocator const& stack_alloc, BOOST_RV_REF( Fn) fn, BOOST_RV_REF( Args) ... args) :
        impl_()
    {
        attributes attrs;
        setup_( stack_alloc, attrs, std::bind( std::forward< Fn >( fn), std::forward< Args >( args) ... ) );
        start_();
    }

    template< typename StackAllocator, typename Fn, class ... Args >
    fiber( boost::allocator_arg_t, StackAllocator const& stack_alloc, attributes const& attrs,
           BOOST_RV_REF( Fn) fn, BOOST_RV_REF( Args) ... args) :
        impl_()
    {
        setup_( stack_alloc, attrs, std::bind( std::forward< Fn >( fn), std::forward< Args >( args) ... ) );
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

    void detach() BOOST_NOEXCEPT;

    void join();

    void interrupt() BOOST_NOEXCEPT;

    template <class PROPS>
    PROPS& properties()
    {
        return fm_properties<PROPS>(impl_.get());
    }
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
