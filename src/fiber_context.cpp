
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/fiber/fiber_context.hpp"

#include "boost/fiber/detail/scheduler.hpp"
#include "boost/fiber/exceptions.hpp"
#include "boost/fiber/properties.hpp"

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

fiber_context::~fiber_context() {
    BOOST_ASSERT( waiting_.empty() );
    delete properties_;
}

fiber_context *
fiber_context::main_fiber() {
    static thread_local fiber_context mf;
    return & mf;
}

void
fiber_context::release() {
    BOOST_ASSERT( is_terminated() );

    std::vector< fiber_context * > waiting;

    // get all waiting fibers
    splk_.lock();
    waiting.swap( waiting_);
    splk_.unlock();

    // notify all waiting fibers
    for ( fiber_context * f : waiting) {
        BOOST_ASSERT( nullptr != f);
        BOOST_ASSERT( ! f->is_terminated() );
        f->set_ready();
    }

    // release fiber-specific-data
    for ( fss_data_t::value_type & data : fss_data_) {
        data.second.do_cleanup();
    }
    fss_data_.clear();
}

bool
fiber_context::join( fiber_context * f) {
    BOOST_ASSERT( nullptr != f);

    detail::spinlock_lock lk( splk_);
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

void *
fiber_context::get_fss_data( void const * vp) const {
    uintptr_t key( reinterpret_cast< uintptr_t >( vp) );
    fss_data_t::const_iterator i( fss_data_.find( key) );

    return fss_data_.end() != i ? i->second.vp : nullptr;
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
        if ( nullptr != data) {
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

void
fiber_context::set_properties( fiber_properties * props) {
    delete properties_;
    properties_ = props;
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
