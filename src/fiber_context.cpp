
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/fiber/fiber_context.hpp"

#include <thread>

#include "boost/fiber/detail/scheduler.hpp"
#include "boost/fiber/exceptions.hpp"

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

fiber_handle
fiber_context::main_fiber() {
    static thread_local fiber_context mf;
    return fiber_handle( & mf);
}

void
fiber_context::release() {
    BOOST_ASSERT( is_terminated() );

    std::vector< fiber_handle > waiting;

    // get all waiting fibers
    splk_.lock();
    waiting.swap( waiting_);
    splk_.unlock();

    // notify all waiting fibers
    for ( fiber_handle p : waiting) {
        p->set_ready();
    }

    // release fiber-specific-data
    for ( fss_data_t::value_type & data : fss_data_) {
        data.second.do_cleanup();
    }
    fss_data_.clear();
}

bool
fiber_context::join( fiber_handle f) {
    BOOST_ASSERT( f);

    std::unique_lock< detail::spinlock > lk( splk_);
    if ( is_terminated() ) {
        return false;
    }
    waiting_.push_back( f);
    return true;
}

void
fiber_context::interruption_blocked( bool blck) noexcept {
    if ( blck) {
        flags_ |= flag_interruption_blocked;
    } else {
        flags_ &= ~flag_interruption_blocked;
    }
}

void
fiber_context::request_interruption( bool req) noexcept {
    if ( req) {
        flags_ |= flag_interruption_requested;
    } else {
        flags_ &= ~flag_interruption_requested;
    }
}

void
fiber_context::thread_affinity( bool req) noexcept {
    // BOOST_ASSERT( 0 == ( flags_.load() & flag_main_fiber) );
    if ( 0 == ( flags_.load() & flag_main_fiber) ) {
        if ( req) {
            flags_ |= flag_thread_affinity;
        } else {
            flags_ &= ~flag_thread_affinity;
        }
    }
}

void *
fiber_context::get_fss_data( void const * vp) const {
    uintptr_t key( reinterpret_cast< uintptr_t >( vp) );
    fss_data_t::const_iterator i( fss_data_.find( key) );

    return fss_data_.end() != i ? i->second.vp : 0;
}

void
fiber_context::set_fss_data( void const * vp,
                             detail::fss_cleanup_function::ptr_t const& cleanup_fn,
                             void * data,
                             bool cleanup_existing) {
    BOOST_ASSERT( cleanup_fn);

    uintptr_t key( reinterpret_cast< uintptr_t >( vp) );
    fss_data_t::iterator i( fss_data_.find( key) );

    if ( fss_data_.end() != i) {
        if( cleanup_existing) {
            i->second.do_cleanup();
        }
        if ( data) {
            fss_data_.insert(
                    i,
                    std::make_pair(
                        key,
                        fss_data( data, cleanup_fn) ) );
        } else {
            fss_data_.erase( i);
        }
    } else {
        fss_data_.insert(
            std::make_pair(
                key,
                fss_data( data, cleanup_fn) ) );
    }
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
