
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
#include <boost/bind.hpp>
#include <boost/config.hpp>
#include <boost/coroutine/all.hpp>
#include <boost/cstdint.hpp>
#include <boost/exception_ptr.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/move/move.hpp>
#include <boost/utility.hpp>

#include <boost/fiber/attributes.hpp>
#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/fiber_context.hpp>
#include <boost/fiber/detail/flags.hpp>
#include <boost/fiber/detail/fss.hpp>
#include <boost/fiber/detail/notify.hpp>

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

namespace coro = boost::coroutines;

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

    struct BOOST_FIBERS_DECL fss_data
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

    // set terminate is only set inside fiber_base::trampoline_()
    void set_terminated_() BOOST_NOEXCEPT
    {
        state_t previous = TERMINATED;
        std::swap( state_, previous);
        BOOST_ASSERT( RUNNING == previous);
    }

    void trampoline_( coro::coroutine< void >::push_type & c)
    {
        BOOST_ASSERT( c);
        BOOST_ASSERT( ! is_terminated() );

        callee_ = & c;
        set_running();
        suspend();

        try
        {
            BOOST_ASSERT( is_running() );
            run();
            BOOST_ASSERT( is_running() );
        }
        catch ( coro::detail::forced_unwind const&)
        {
            set_terminated_();
            release();
            throw;
        }
        catch (...)
        { except_ = current_exception(); }

        set_terminated_();
        release();
        suspend();

        BOOST_ASSERT_MSG( false, "fiber already terminated");
    }

protected:
    state_t                                 state_;
    int                                     flags_;
    int                                     priority_;
    coro::coroutine< void >::pull_type      caller_;
    coro::coroutine< void >::push_type  *   callee_;
    exception_ptr                           except_;
    std::vector< ptr_t >                    waiting_;

    void release();

    virtual void run() = 0;

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

    template< typename StackAllocator, typename Allocator >
    fiber_base( attributes const& attr, StackAllocator const& stack_alloc, Allocator const& alloc) :
        fss_data_(),
        state_( READY),
        flags_( 0),
        priority_( 0),
        caller_(),
        callee_( 0),
        except_(),
        waiting_()
    {
        BOOST_ASSERT( ! caller_);
        BOOST_ASSERT( ! callee_);

        typedef typename Allocator::template rebind<
            coro::coroutine< void >::pull_type
        >::other    allocator_t;

        caller_ = coro::coroutine< void >::pull_type(
                boost::bind( & fiber_base::trampoline_, this, _1),
                coro::attributes(
                    attr.size,
                    fpu_preserved == attr.preserve_fpu
                    ? coro::fpu_preserved
                    : coro::fpu_not_preserved),
                stack_alloc,
                allocator_t( alloc) );

        set_ready(); // fiber is setup and now ready to run

        BOOST_ASSERT( caller_);
        BOOST_ASSERT( callee_);
    }

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

    void set_ready() BOOST_NOEXCEPT
    {
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
        state_t previous = WAITING;
        std::swap( state_, previous);
        BOOST_ASSERT( RUNNING == previous);
    }

    void * get_fss_data( void const* vp) const;

    void set_fss_data(
        void const* vp,
        fss_cleanup_function::ptr_t const& cleanup_fn,
        void * data,
        bool cleanup_existing);

    bool has_exception() const BOOST_NOEXCEPT
    { return except_; }

    void rethrow() const;
};

}}}

# if defined(BOOST_MSVC)
# pragma warning(pop)
# endif

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DETAIL_FIBER_BASE_H
