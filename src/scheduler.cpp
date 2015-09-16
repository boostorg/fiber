
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/fiber/scheduler.hpp"

#include <boost/assert.hpp>

#include "boost/fiber/context.hpp"

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

void
scheduler::resume_( context * actx, context * ctx) {
    BOOST_ASSERT( nullptr != actx);
    BOOST_ASSERT( nullptr != ctx);
    // fiber next-to-run is same as current active-fiber
    // this might happen in context of this_fiber::yield() 
    if ( actx == ctx) {
        return;
    }
    // pass new fiber the scheduler of the current active fiber
    // this might be necessary if the new fiber was migrated
    // from another thread
    // FIXME: mabye better don in the sched-algorithm (knows
    //        if fiber was migrated)
    //        -> performance issue?
    ctx->set_scheduler( actx->get_scheduler() );
    // assign new fiber to active-fiber
    context::active( ctx);
    // resume active-fiber == ctx
    ctx->resume();
}

context *
scheduler::get_next_() noexcept {
    BOOST_ASSERT( ! ready_queue_.empty() );
    context * ctx( & ready_queue_.front() );
    ready_queue_.pop_front();
    return ctx;
}

void
scheduler::release_terminated_() {
    terminated_queue_t::iterator e( terminated_queue_.end() );
    for ( terminated_queue_t::iterator i( terminated_queue_.begin() );
            i != e;) {
        context * ctx( & ( * i) );
        i = terminated_queue_.erase( i);
        intrusive_ptr_release( ctx);
    }
}

scheduler::scheduler() noexcept :
    main_ctx_( nullptr),
    dispatching_ctx_(),
    ready_queue_(),
    terminated_queue_(),
    shutdown_( false) {
}

scheduler::~scheduler() noexcept {
    BOOST_ASSERT( nullptr != main_ctx_);
    BOOST_ASSERT( nullptr != dispatching_ctx_.get() );
    BOOST_ASSERT( context::active() == main_ctx_);

    // signal dispatching context termination
    shutdown_ = true;
    // resume pending fibers
    resume_( main_ctx_, get_next_() );
    // deallocate dispatching-context
    dispatching_ctx_.reset();
    // set main-context to nullptr
    main_ctx_ = nullptr;
}

void
scheduler::set_main_context( context * main_ctx) noexcept {
    BOOST_ASSERT( nullptr != main_ctx);
    main_ctx_ = main_ctx;
}

void
scheduler::set_dispatching_context( intrusive_ptr< context > dispatching_ctx) noexcept {
    BOOST_ASSERT( dispatching_ctx);
    dispatching_ctx_.swap( dispatching_ctx);
    // add dispatching context to ready-queue
    // so it is the first element in the ready-queue
    // if the main context tries to suspend the first time
    // the dispatching context is resumed and
    // scheduler::dispatch() is executed
    ready_queue_.push_back( * dispatching_ctx_.get() );
}

void
scheduler::dispatch() {
    while ( ! shutdown_) {
        release_terminated_();
    }
    release_terminated_();
    resume_( dispatching_ctx_.get(), main_ctx_);
}

void
scheduler::set_terminated( context * ctx) noexcept {
    BOOST_ASSERT( nullptr != ctx);
    BOOST_ASSERT( ! ctx->ready_is_linked() );
    BOOST_ASSERT( ! ctx->wait_is_linked() );
    terminated_queue_.push_back( * ctx);
}

void
scheduler::re_schedule( context * actx) noexcept {
    BOOST_ASSERT( nullptr != actx);
    resume_( actx, get_next_() );
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
