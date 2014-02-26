
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_DETAIL_FIBER_BASE_H
#define BOOST_FIBERS_DETAIL_FIBER_BASE_H

#include <cstddef>
#include <map>
#include <vector>

#include <boost/assert.hpp>
#include <boost/atomic.hpp>
#include <boost/config.hpp>
#include <boost/cstdint.hpp>
#include <boost/exception_ptr.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/move/move.hpp>
#include <boost/utility.hpp>

#include <boost/fiber/attributes.hpp>
#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/flags.hpp>
#include <boost/fiber/detail/fss.hpp>
#include <boost/fiber/detail/notify.hpp>
#include <boost/fiber/detail/spinlock.hpp>

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

protected:
    atomic< state_t >       state_;
    atomic< int >           flags_;
    atomic< int >           priority_;
    exception_ptr           except_;
    spinlock                splk_;
    std::vector< ptr_t >    waiting_;

    void release();

public:
    fiber_base();

    virtual ~fiber_base();

    id get_id() const BOOST_NOEXCEPT
    { return id( const_cast< fiber_base * >( this) ); }

    int priority() const BOOST_NOEXCEPT
    { return priority_; }

    void priority( int prio) BOOST_NOEXCEPT
    { priority_ = prio; }

    bool join( ptr_t const&);

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

    void set_terminated() BOOST_NOEXCEPT;

    void set_ready() BOOST_NOEXCEPT;

    void set_running() BOOST_NOEXCEPT;

    void set_waiting() BOOST_NOEXCEPT;

    void * get_fss_data( void const* vp) const;

    void set_fss_data(
        void const* vp,
        fss_cleanup_function::ptr_t const& cleanup_fn,
        void * data,
        bool cleanup_existing);

    bool has_exception() const BOOST_NOEXCEPT
    { return except_; }

    void rethrow() const;

    virtual void resume() = 0;

    virtual void suspend() = 0;
};

}}}

# if defined(BOOST_MSVC)
# pragma warning(pop)
# endif

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DETAIL_FIBER_BASE_H
