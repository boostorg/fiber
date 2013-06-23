
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_DETAIL_FIBER_BASE_H
#define BOOST_FIBERS_DETAIL_FIBER_BASE_H

#include <cstddef>
#include <iostream>
#include <map>
#include <vector>

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/cstdint.hpp>
#include <boost/exception_ptr.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/utility.hpp>

#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/fiber_context.hpp>
#include <boost/fiber/detail/flags.hpp>
#include <boost/fiber/detail/fss.hpp>
#include <boost/fiber/detail/notify.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace detail {

struct forced_unwind {};

class BOOST_FIBERS_DECL fiber_base : public notify
{
public:
    typedef intrusive_ptr< fiber_base >     ptr_t;

private:
    enum state_t
    {
        READY = 0,
        RUNNING,
        WAITING,
        TERMINATED
    };

    struct fss_data
    {
        void                       *   vp;
        fss_cleanup_function::ptr_t     cleanup_function;

        fss_data() :
            vp( 0), cleanup_function( 0)
        {}

        fss_data(
                void * vp_,
                fss_cleanup_function::ptr_t const& fn) :
            vp( vp_), cleanup_function( fn)
        { BOOST_ASSERT( cleanup_function); }

        void do_cleanup()
        { ( * cleanup_function)( vp); }
    };

    typedef std::map< uintptr_t, fss_data >   fss_data_t;

    fss_data_t              fss_data_;

protected:
    state_t                 state_;
    int                     flags_;
    int                     priority_;
    fiber_context           caller_;
    fiber_context           callee_;
    exception_ptr           except_;
    std::vector< ptr_t >    waiting_;

    virtual void unwind_stack() = 0;

    void release();

public:
    class id
    {
    private:
        friend class fiber_base;

        fiber_base const*   impl_;

    public:
        explicit id( fiber_base const* impl) BOOST_NOEXCEPT :
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

    fiber_base( fiber_context::ctx_fn fn,
                stack_context * stack_ctx,
                bool preserve_fpu);

    virtual ~fiber_base();

    id get_id() const BOOST_NOEXCEPT
    { return id( this); }

    int priority() const BOOST_NOEXCEPT
    { return priority_; }

    void priority( int prio) BOOST_NOEXCEPT
    { priority_ = prio; }

    void resume();

    void suspend();

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
    { return TERMINATED == state_; }

    bool is_ready() const BOOST_NOEXCEPT
    { return READY == state_; }

    bool is_running() const BOOST_NOEXCEPT
    { return RUNNING == state_; }

    bool is_waiting() const BOOST_NOEXCEPT
    { return WAITING == state_; }

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
        state_t previous = TERMINATED;
        std::swap( state_, previous);
        BOOST_ASSERT( RUNNING == previous);
        //BOOST_ASSERT( RUNNING == previous || READY == previous);
    }

    void set_ready() BOOST_NOEXCEPT
    {
#if 0
    // this fiber calls set_ready(): - only transition from WAITING (wake-up)
    //                               - or transition from RUNNING (yield) allowed
    // other fiber calls set_ready(): - only if this fiber was joinig other fiber
    //                                - if this fiber was not interrupted then this fiber
    //                                  should in WAITING
    //                                - if this fiber was interrupted the this fiber might
    //                                  be in READY, RUNNING or already in
    //                                  TERMINATED
    for (;;)
    {
        int expected = WAITING;
        bool result = state_.compare_exchange_strong( expected, READY);
        if ( result || TERMINATED == expected || READY == expected) return;
        expected = RUNNING;
        result = state_.compare_exchange_strong( expected, READY);
        if ( result || TERMINATED == expected || READY == expected) return;
    }
#endif
        state_t previous = READY;
        std::swap( state_, previous);
        BOOST_ASSERT( WAITING == previous || RUNNING == previous || READY == previous);
    }

    void set_running() BOOST_NOEXCEPT
    {
        state_t previous = RUNNING;
        std::swap( state_, previous);
        BOOST_ASSERT( READY == previous);
    }

    void set_waiting() BOOST_NOEXCEPT
    {
        // other thread could have called set_ready() before
        // case: - this fiber has joined another fiber running in another thread,
        //       - other fiber terminated and releases its joining fibers
        //       - this fiber was interrupted and therefore resumed and
        //         throws fiber_interrupted
        //       - fiber_interrupted was catched and swallowed
        //       - other fiber calls set_ready() on this fiber happend before
        //         this fiber calls set_waiting()
        //       - this fiber might wait on some sync. primitive calling
        //         set_waiting()
        state_t previous = WAITING;
        std::swap( state_, previous);
        BOOST_ASSERT( RUNNING == previous);
        //BOOST_ASSERT( RUNNING == previous || READY == previous);
    }

    bool has_exception() const BOOST_NOEXCEPT
    { return except_; }

    void rethrow() const;

    void * get_fss_data( void const* vp) const;

    void set_fss_data(
        void const* vp,
        fss_cleanup_function::ptr_t const& cleanup_fn,
        void * data,
        bool cleanup_existing);
};

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DETAIL_FIBER_BASE_H
