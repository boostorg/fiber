
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_DETAIL_WORKER_FIBER_H
#define BOOST_FIBERS_DETAIL_WORKER_FIBER_H

#include <cstddef>
#include <map>
#include <vector>

#include <boost/assert.hpp>
#include <boost/atomic.hpp>
#include <boost/config.hpp>
#include <boost/cstdint.hpp>
#include <boost/coroutine/symmetric_coroutine.hpp>
#include <boost/exception_ptr.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/move/move.hpp>
#include <boost/utility.hpp>

#include <boost/fiber/attributes.hpp>
#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/fiber_base.hpp>
#include <boost/fiber/detail/flags.hpp>
#include <boost/fiber/detail/fss.hpp>
#include <boost/fiber/detail/spinlock.hpp>
#include <boost/fiber/exceptions.hpp>

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

class BOOST_FIBERS_DECL worker_fiber : public fiber_base
{
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
    typedef coro::symmetric_coroutine<
        void
    >                                          coro_t;

    fss_data_t              fss_data_;
    worker_fiber        *   nxt_;
    clock_type::time_point  tp_;

    void trampoline_( coro_t::yield_type &);

protected:
    coro_t::yield_type          *   callee_;
    coro_t::call_type               caller_;
    atomic< state_t >               state_;
    atomic< int >                   flags_;
    atomic< int >                   priority_;
    exception_ptr                   except_;
    spinlock                        splk_;
    std::vector< worker_fiber * >   waiting_;

    void release();

    virtual void run() = 0;

public:
    worker_fiber( attributes const&);

    virtual ~worker_fiber();

    id get_id() const BOOST_NOEXCEPT
    { return id( const_cast< worker_fiber * >( this) ); }

    int priority() const BOOST_NOEXCEPT
    { return priority_; }

    void priority( int prio) BOOST_NOEXCEPT
    { priority_ = prio; }

    bool join( worker_fiber *);

    bool detached() const BOOST_NOEXCEPT
    { return 0 != ( flags_.load() & flag_detached); }

    void detach() BOOST_NOEXCEPT
    { flags_ |= flag_detached; }

    bool interruption_blocked() const BOOST_NOEXCEPT
    { return 0 != ( flags_.load() & flag_interruption_blocked); }

    void interruption_blocked( bool blck) BOOST_NOEXCEPT;

    bool interruption_requested() const BOOST_NOEXCEPT
    { return 0 != ( flags_.load() & flag_interruption_requested); }

    void request_interruption( bool req) BOOST_NOEXCEPT;

    bool thread_affinity() const BOOST_NOEXCEPT
    { return 0 != ( flags_.load() & flag_thread_affinity); }

    void thread_affinity( bool req) BOOST_NOEXCEPT;

    bool is_terminated() const BOOST_NOEXCEPT
    { return TERMINATED == state_; }

    bool is_ready() const BOOST_NOEXCEPT
    { return READY == state_; }

    bool is_running() const BOOST_NOEXCEPT
    { return RUNNING == state_; }

    bool is_waiting() const BOOST_NOEXCEPT
    { return WAITING == state_; }

    void set_terminated() BOOST_NOEXCEPT
    {
        state_t previous = state_.exchange( TERMINATED);
        BOOST_ASSERT( RUNNING == previous);
    }

    void set_ready() BOOST_NOEXCEPT
    {
        state_t previous = state_.exchange( READY);
        BOOST_ASSERT( WAITING == previous || RUNNING == previous || READY == previous);
    }

    void set_running() BOOST_NOEXCEPT
    {
        state_t previous = state_.exchange( RUNNING);
        BOOST_ASSERT( READY == previous);
    }

    void set_waiting() BOOST_NOEXCEPT
    {
        state_t previous = state_.exchange( WAITING);
        BOOST_ASSERT( RUNNING == previous);
    }

    void * get_fss_data( void const* vp) const;

    void set_fss_data(
        void const* vp,
        fss_cleanup_function::ptr_t const& cleanup_fn,
        void * data,
        bool cleanup_existing);

    exception_ptr exception() const BOOST_NOEXCEPT
    { return except_; }

    void resume( worker_fiber * f)
    {
        if ( 0 == f)
        {
            BOOST_ASSERT( caller_);
            BOOST_ASSERT( is_running() ); // set by the scheduler-algorithm

            // called from main-fiber
            caller_();
        }
        else
        {
            // caller from worker-fiber f
            BOOST_ASSERT( caller_);
            BOOST_ASSERT( is_running() ); // set by the scheduler-algorithm
            BOOST_ASSERT( f->callee_);

            ( * f->callee_)( caller_);
        }
    }

    void suspend()
    {
        BOOST_ASSERT( callee_);
        BOOST_ASSERT( * callee_);

        ( * callee_)();

        BOOST_ASSERT( is_running() ); // set by the scheduler-algorithm
    }

    worker_fiber * next() const
    { return nxt_; }

    void next( worker_fiber * nxt)
    { nxt_ = nxt; }

    void next_reset()
    { nxt_ = 0; }

    clock_type::time_point const& time_point() const
    { return tp_; }

    void time_point( clock_type::time_point const& tp)
    { tp_ = tp; }

    void time_point_reset()
    { tp_ = (clock_type::time_point::max)(); }

    virtual void deallocate() = 0;
};

}}}

# if defined(BOOST_MSVC)
# pragma warning(pop)
# endif

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DETAIL_WORKER_FIBER_H
