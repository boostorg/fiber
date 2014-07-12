
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
#include <boost/utility.hpp>
#include <boost/utility/explicit_operator_bool.hpp>

#include <boost/fiber/attributes.hpp>
#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/setup.hpp>
#include <boost/fiber/detail/trampoline.hpp>
#include <boost/fiber/detail/worker_fiber.hpp>
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

    BOOST_MOVABLE_BUT_NOT_COPYABLE( fiber);

    void start_fiber_();

public:
    typedef detail::worker_fiber::id    id;

    fiber() BOOST_NOEXCEPT :
        impl_()
    {}

    explicit fiber( detail::worker_fiber * impl) BOOST_NOEXCEPT :
        impl_( impl)
    {}

#ifdef BOOST_NO_RVALUE_REFERENCES
    template< typename Fn >
    explicit fiber( Fn fn, attributes const& attrs = attributes(),
                    stack_allocator const& stack_alloc = stack_allocator() ) :
        impl_()
    {
        typename coro_t::call_type coro( detail::trampoline< Fn >, attrs, stack_alloc); 
        detail::setup< Fn > s( fn, & coro);
        impl_.reset( s.allocate() );
        BOOST_ASSERT( impl_);

        start_fiber_();
    }

    template< typename Fn, typename StackAllocator >
    explicit fiber( Fn fn, attributes const& attrs,
                    StackAllocator const& stack_alloc) :
        impl_()
    {
        typename coro_t::call_type coro( detail::trampoline< Fn >, attrs, stack_alloc); 
        detail::setup< Fn > s( fn, & coro);
        impl_.reset( s.allocate() );
        BOOST_ASSERT( impl_);

        start_fiber_();
    }
#endif

    template< typename Fn >
    explicit fiber( BOOST_RV_REF( Fn) fn, attributes const& attrs = attributes(),
                    stack_allocator const& stack_alloc = stack_allocator() ) :
        impl_()
    {
        typename coro_t::call_type coro( detail::trampoline< Fn >, attrs, stack_alloc); 
#ifdef BOOST_NO_RVALUE_REFERENCES
        detail::setup< Fn > s( fn, & coro);
#else
        detail::setup< Fn > s( forward< Fn >( fn), & coro);
#endif
        impl_.reset( s.allocate() );
        BOOST_ASSERT( impl_);

        start_fiber_();
    }

    template< typename Fn, typename StackAllocator >
    explicit fiber( BOOST_RV_REF( Fn) fn, attributes const& attrs,
                    StackAllocator const& stack_alloc) :
        impl_()
    {
        typename coro_t::call_type coro( detail::trampoline< Fn >, attrs, stack_alloc); 
#ifdef BOOST_NO_RVALUE_REFERENCES
        detail::setup< Fn > s( fn, & coro);
#else
        detail::setup< Fn > s( forward< Fn >( fn), & coro);
#endif
        impl_.reset( s.allocate() );
        BOOST_ASSERT( impl_);

        start_fiber_();
    }

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
