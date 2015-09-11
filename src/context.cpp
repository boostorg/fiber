
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/fiber/context.hpp"

#include "boost/fiber/algorithm.hpp"
#include "boost/fiber/exceptions.hpp"
#include "boost/fiber/fiber.hpp"
#include "boost/fiber/properties.hpp"
#include "boost/fiber/scheduler.hpp"

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

static context * main_context() {
    static thread_local context mc;
    static thread_local scheduler sched( & mc);
    mc.set_scheduler( & sched);
    return & mc;
}

thread_local context *
context::active_ = main_context();

context *
context::active() noexcept {
    return active_;
}

void
context::active( context * active) noexcept {
    BOOST_ASSERT( nullptr != active);
    active_ = active;
}

context::~context() {
    BOOST_ASSERT( wait_queue_.empty() );
    delete properties_;
}

void
context::release() {
    BOOST_ASSERT( is_terminated() );

    wait_queue_t tmp;

    // get all waiting fibers
    splk_.lock();
    tmp.swap( wait_queue_);
    splk_.unlock();

    // notify all waiting fibers
    for ( context * f : tmp) {
        do_signal( f);
    }

    // release fiber-specific-data
    for ( fss_data_t::value_type & data : fss_data_) {
        data.second.do_cleanup();
    }
    fss_data_.clear();
}

bool
context::join( context * f) {
    BOOST_ASSERT( nullptr != f);

    detail::spinlock_lock lk( splk_);
    if ( is_terminated() ) {
        return false;
    }
    f->set_waiting();
    wait_queue_.push_back( f);
    return true;
}

void
context::interruption_blocked( bool blck) noexcept {
    if ( blck) {
        flags_ |= flag_interruption_blocked;
    } else {
        flags_ &= ~flag_interruption_blocked;
    }
}

void
context::request_interruption( bool req) noexcept {
    if ( req) {
        flags_ |= flag_interruption_requested;
    } else {
        flags_ &= ~flag_interruption_requested;
    }
}

void *
context::get_fss_data( void const * vp) const {
    uintptr_t key( reinterpret_cast< uintptr_t >( vp) );
    fss_data_t::const_iterator i( fss_data_.find( key) );

    return fss_data_.end() != i ? i->second.vp : nullptr;
}

void
context::set_fss_data( void const * vp,
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
context::set_properties( fiber_properties * props) {
    delete properties_;
    properties_ = props;
}

void
context::do_spawn( fiber const& f) {
    BOOST_ASSERT( nullptr != scheduler_);
    BOOST_ASSERT( this == active_);

    scheduler_->spawn( f.impl_.get() );
}

void
context::do_schedule() {
    BOOST_ASSERT( nullptr != scheduler_);
    BOOST_ASSERT( this == active_);

    scheduler_->run( this);
}

bool
context::do_wait_until( std::chrono::steady_clock::time_point const& time_point) {
    BOOST_ASSERT( nullptr != scheduler_);
    BOOST_ASSERT( this == active_);

    return scheduler_->wait_until( this, time_point);
}

void
context::do_yield() {
    BOOST_ASSERT( nullptr != scheduler_);
    BOOST_ASSERT( this == active_);

    scheduler_->yield( this);
}

void
context::do_join( context * f) {
    BOOST_ASSERT( nullptr != scheduler_);
    BOOST_ASSERT( this == active_);
    BOOST_ASSERT( nullptr != f);
    BOOST_ASSERT( f != active_);

    if ( f->join( this) ) {
        scheduler_->run( this);
    }
}

void
context::do_signal( context * f) {
    BOOST_ASSERT( nullptr != scheduler_);
    BOOST_ASSERT( this == active_);
    BOOST_ASSERT( nullptr != f);

    if ( scheduler_ == f->scheduler_) {
        // local scheduler
        scheduler_->signal( f);
    } else {
        // scheduler in another thread
        f->scheduler_->remote_signal( f);
    }
}

std::size_t
context::do_ready_fibers() const noexcept {
    BOOST_ASSERT( nullptr != scheduler_);
    BOOST_ASSERT( this == active_);

    return scheduler_->ready_fibers();
}

void
context::do_set_sched_algo( std::unique_ptr< sched_algorithm > algo) {
    BOOST_ASSERT( nullptr != scheduler_);
    BOOST_ASSERT( this == active_);

    scheduler_->set_sched_algo( std::move( algo) );
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
