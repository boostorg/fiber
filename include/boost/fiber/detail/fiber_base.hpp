
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_DETAIL_FIBER_BASE_H
#define BOOST_FIBERS_DETAIL_FIBER_BASE_H

#include <cstddef>
#include <iostream>
#include <vector>

#include <boost/assert.hpp>
#include <boost/atomic.hpp>
#include <boost/chrono/system_clocks.hpp>
#include <boost/config.hpp>
#include <boost/context/fcontext.hpp>
#include <boost/cstdint.hpp>
#include <boost/exception_ptr.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/utility.hpp>

#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/flags.hpp>
#include <boost/fiber/detail/spinlock.hpp>
#include <boost/fiber/detail/states.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace detail {

class BOOST_FIBERS_DECL fiber_base : private noncopyable
{
public:
    typedef intrusive_ptr< fiber_base >           ptr_t;

private:
    template< typename X, typename Y, typename Z >
    friend class fiber_object;

    atomic< std::size_t >   use_count_;
    atomic< state_t >       state_;
    atomic< int >           flags_;
    atomic< int >           priority_;
    context::fcontext_t     caller_;
    context::fcontext_t *   callee_;
    exception_ptr           except_;
    spinlock                joining_mtx_;
    std::vector< ptr_t >    joining_;

protected:
    virtual void deallocate_object() = 0;
    virtual void unwind_stack() = 0;

public:
    class id
    {
    private:
        friend class fiber_base;

        fiber_base::ptr_t   impl_;

        explicit id( fiber_base::ptr_t const& impl) BOOST_NOEXCEPT :
            impl_( impl)
        {}

    public:
        id() BOOST_NOEXCEPT :
            impl_()
        {}

        bool operator==( id const& other) const BOOST_NOEXCEPT
        { return impl_ == other.impl_; }

        bool operator!=( id const& other) const BOOST_NOEXCEPT
        { return impl_ != other.impl_; }
        
        bool operator<( id const& other) const BOOST_NOEXCEPT
        { return impl_ < other.impl_; }
        
        bool operator>( id const& other) const BOOST_NOEXCEPT
        { return other.impl_ < impl_; }
        
        bool operator<=( id const& other) const BOOST_NOEXCEPT
        { return ! ( * this > other); }
        
        bool operator>=( id const& other) const BOOST_NOEXCEPT
        { return ! ( * this < other); }

        template< typename charT, class traitsT >
        friend std::basic_ostream< charT, traitsT > &
        operator<<( std::basic_ostream< charT, traitsT > & os, id const& other)
        {
            if ( 0 != other.impl_)
                return os << other.impl_;
            else
                return os << "{not-valid}";
        }

        operator bool() const BOOST_NOEXCEPT
        { return 0 != impl_; }

        bool operator!() const BOOST_NOEXCEPT
        { return 0 == impl_; }
    };

    fiber_base( context::fcontext_t *, bool);

    virtual ~fiber_base() {}

    id get_id() const BOOST_NOEXCEPT
    { return id( ptr_t( const_cast< fiber_base * >( this) ) ); }

    int priority() const BOOST_NOEXCEPT
    { return priority_; }

    void priority( int prio) BOOST_NOEXCEPT
    { priority_ = prio; }

    void resume();

    void suspend();

    void yield();

    void release();

    bool join( ptr_t const&);

    bool force_unwind() const BOOST_NOEXCEPT
    { return 0 != ( flags_ & flag_force_unwind); }

    bool unwind_requested() const BOOST_NOEXCEPT
    { return 0 != ( flags_ & flag_unwind_stack); }

    bool preserve_fpu() const BOOST_NOEXCEPT
    { return 0 != ( flags_ & flag_preserve_fpu); }

    bool interruption_enabled() const BOOST_NOEXCEPT
    { return 0 == ( flags_ & flag_interruption_blocked); }

    bool interruption_blocked() const BOOST_NOEXCEPT
    { return 0 != ( flags_ & flag_interruption_blocked); }

    void interruption_blocked( bool blck) BOOST_NOEXCEPT
    {
        if ( blck)
            flags_ |= flag_interruption_blocked;
        else
            flags_ &= ~flag_interruption_blocked;
    }

    bool interruption_requested() const BOOST_NOEXCEPT
    { return 0 != ( flags_ & flag_interruption_requested); }

    void request_interruption( bool req) BOOST_NOEXCEPT
    {
        if ( req)
            flags_ |= flag_interruption_requested;
        else
            flags_ &= ~flag_interruption_requested;
    }

    bool is_terminated() const BOOST_NOEXCEPT
    { return state_terminated == state_; }

    bool is_ready() const BOOST_NOEXCEPT
    { return state_ready == state_; }

    bool is_running() const BOOST_NOEXCEPT
    { return state_running == state_; }

    bool is_waiting() const BOOST_NOEXCEPT
    { return state_waiting == state_; }

    // set terminate is only set inside fiber_object::exec()
    // it is set after the fiber-function was left == at the end of exec()
    void set_terminated() BOOST_NOEXCEPT
    {
        // other thread could have called set_ready() before
        // case: - this fiber has joined another fiber running in another thread,
        //       - other fiber terminated and releases its joining fibers
        //       - this fiber was interrupted before and therefore resumed
        //         and throws fiber_interrupted
        //       - fiber_interrupted was not catched and swallowed
        //       - other fiber calls set_ready() on this fiber happend before this
        //         fiber calls set_terminated()
        //       - this fiber stack gets unwound and set_terminated() is called at the end
        state_t previous = state_.exchange( state_terminated, memory_order_release);
        BOOST_ASSERT( state_running == previous || state_ready == previous);
    }

    void set_ready() BOOST_NOEXCEPT
    {
        // this fiber calls set_ready(): - only transition from state_waiting (wake-up)
        //                               - or transition from state_running (yield) allowed
        // other fiber calls set_ready(): - only if this fiber was joinig other fiber
        //                                - if this fiber was not interrupted then this fiber
        //                                  should in state_waiting
        //                                - if this fiber was interrupted the this fiber might
        //                                  be in state_ready, state_running or already in
        //                                  state_terminated
        for (;;)
        {
            state_t expected = state_waiting;
            bool result = state_.compare_exchange_strong( expected, state_ready, memory_order_release);
            if ( result || state_terminated == expected || state_ready == expected) return;
            expected = state_running;
            result = state_.compare_exchange_strong( expected, state_ready, memory_order_release);
            if ( result || state_terminated == expected || state_ready == expected) return;
        }
    }

    void set_running() BOOST_NOEXCEPT
    {
        state_t previous = state_.exchange( state_running, memory_order_release);
        BOOST_ASSERT( state_ready == previous);
    }

    void set_waiting() BOOST_NOEXCEPT
    {
        // other thread could have called set_ready() before
        // case: - this fiber has joined another fiber running in another thread,
        //       - other fiber terminated and releases its joining fibers
        //       - this fiber was interrupted and therefore resumed and
        //         throws fiber_interrupted
        //       - fiber_interrupted was catched and swallowed
        //       - other fiber calls set_ready() on this fiber happend before this
        //         fiber calls set_waiting()
        //       - this fiber might wait on some sync. primitive calling set_waiting()
        state_t previous = state_.exchange( state_waiting, memory_order_release);
        BOOST_ASSERT( state_running == previous || state_ready == previous);
    }

    state_t state() const BOOST_NOEXCEPT
    { return state_; }

    friend inline void intrusive_ptr_add_ref( fiber_base * p) BOOST_NOEXCEPT
    { p->use_count_.fetch_add( 1, memory_order_relaxed); }

    friend inline void intrusive_ptr_release( fiber_base * p)
    {
        if ( 1 == p->use_count_.fetch_sub( 1, memory_order_release) )
        {
            atomic_thread_fence( memory_order_acquire);
            p->deallocate_object();
        }
    }
};

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DETAIL_FIBER_BASE_H
