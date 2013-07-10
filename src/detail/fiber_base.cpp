
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/fiber/detail/fiber_base.hpp"

#include <boost/exception_ptr.hpp>
#include <boost/foreach.hpp>
#include <boost/thread/locks.hpp>

#include "boost/fiber/detail/scheduler.hpp"

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace detail {

fiber_base::fiber_base( fiber_context::ctx_fn fn,
                        stack_context * stack_ctx,
                        bool preserve_fpu) :
    fss_data_(),
    state_( READY),
    flags_( 0),
    priority_( 0),
    caller_(),
    callee_( fn, stack_ctx),
    except_(),
    waiting_()
{ if ( preserve_fpu) flags_ |= flag_preserve_fpu; }

fiber_base::~fiber_base()
{
    BOOST_ASSERT( is_terminated() );
    BOOST_ASSERT( waiting_.empty() );
}

void
fiber_base::resume()
{
    BOOST_ASSERT( is_running() );

    caller_.jump( callee_, 0, preserve_fpu() );

    if ( has_exception() ) rethrow();
}

void
fiber_base::suspend()
{
    callee_.jump( caller_, 0, preserve_fpu() );

    BOOST_ASSERT( is_running() );

    if ( unwind_requested() ) throw forced_unwind();
}

void
fiber_base::release()
{
    BOOST_ASSERT( is_terminated() );

    std::vector< ptr_t > waiting;

    // get all waiting fibers
    waiting.swap( waiting_);

    // notify all waiting fibers
    BOOST_FOREACH( fiber_base::ptr_t p, waiting)
    { p->set_ready(); }

    // release all fiber-specific-pointers
    BOOST_FOREACH( fss_data_t::value_type & data, fss_data_)
    { data.second.do_cleanup(); }
}

bool
fiber_base::join( ptr_t const& p)
{
    // protect against concurrent access to waiting_
    if ( is_terminated() ) return false;
    waiting_.push_back( p);
    return true;
}

void
fiber_base::rethrow() const
{
    BOOST_ASSERT( has_exception() );

    rethrow_exception( except_);
}

void *
fiber_base::get_fss_data( void const* vp) const
{
    uintptr_t key( reinterpret_cast< uintptr_t >( vp) );
    fss_data_t::const_iterator i( fss_data_.find( key) );

    return fss_data_.end() != i ? i->second.vp : 0;
}

void
fiber_base::set_fss_data(
    void const* vp,
    fss_cleanup_function::ptr_t const& cleanup_fn,
    void * data, bool cleanup_existing)
{
    BOOST_ASSERT( cleanup_fn);

    uintptr_t key( reinterpret_cast< uintptr_t >( vp) );
    fss_data_t::iterator i( fss_data_.find( key) );

    if ( fss_data_.end() != i)
    {
        if( cleanup_existing) i->second.do_cleanup();
        if ( data)
            fss_data_.insert(
                    i,
                    std::make_pair(
                        key,
                        fss_data( data, cleanup_fn) ) );
        else fss_data_.erase( i);
    }
    else
        fss_data_.insert(
            std::make_pair(
                key,
                fss_data( data, cleanup_fn) ) );
}

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
