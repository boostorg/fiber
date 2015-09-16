
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/fiber/scheduler.hpp"

#include <chrono>

#include <boost/assert.hpp>

#include "boost/fiber/context.hpp"

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

void
scheduler::resume_( context * active_ctx, context * ctx) {
    BOOST_ASSERT( nullptr != active_ctx);
    BOOST_ASSERT( nullptr != ctx);
    BOOST_ASSERT( active_ctx->get_scheduler() == ctx->get_scheduler() );
    // fiber next-to-run is same as current active-fiber
    // this might happen in context of this_fiber::yield() 
    if ( active_ctx == ctx) {
        return;
    }
    // assign new fiber to active-fiber
    context::active( ctx);
    // resume active-fiber == ctx
    ctx->resume();
}

context *
scheduler::get_next_() noexcept {
    context * ctx( nullptr);
    if ( ! ready_queue_.empty() ) {
        ctx = & ready_queue_.front();
        ready_queue_.pop_front();
    }
    return ctx;
}

void
scheduler::release_terminated_() {
    terminated_queue_t::iterator e( terminated_queue_.end() );
    for ( terminated_queue_t::iterator i( terminated_queue_.begin() );
            i != e;) {
        context * ctx = & ( * i);
        i = terminated_queue_.erase( i);
        intrusive_ptr_release( ctx);
    }
}

scheduler::scheduler() noexcept :
    main_ctx_( nullptr),
    dispatcher_ctx_(),
    ready_queue_(),
    terminated_queue_(),
    shutdown_( false),
    ready_queue_ev_() {
}

scheduler::~scheduler() noexcept {
    BOOST_ASSERT( nullptr != main_ctx_);
    BOOST_ASSERT( nullptr != dispatcher_ctx_.get() );
    BOOST_ASSERT( context::active() == main_ctx_);

    // signal dispatcher context termination
    shutdown_ = true;
    // resume pending fibers
    resume_( main_ctx_, get_next_() );
    // deallocate dispatcher-context
    dispatcher_ctx_.reset();
    // set main-context to nullptr
    main_ctx_ = nullptr;
}

void
scheduler::set_main_context( context * main_ctx) noexcept {
    BOOST_ASSERT( nullptr != main_ctx);
    main_ctx_ = main_ctx;
    main_ctx_->set_scheduler( this);
}

void
scheduler::set_dispatcher_context( intrusive_ptr< context > dispatcher_ctx) noexcept {
    BOOST_ASSERT( dispatcher_ctx);
    dispatcher_ctx_.swap( dispatcher_ctx);
    // add dispatcher context to ready-queue
    // so it is the first element in the ready-queue
    // if the main context tries to suspend the first time
    // the dispatcher context is resumed and
    // scheduler::dispatch() is executed
    dispatcher_ctx_->set_scheduler( this);
    ready_queue_.push_back( * dispatcher_ctx_.get() );
}

void
scheduler::dispatch() {
    while ( ! shutdown_) {
        release_terminated_();
        context * ctx( nullptr);
        // loop till we get next ready context
        while ( nullptr == ( ctx = get_next_() ) ) {
            // FIXME: move context' from remote ready-queue to local ready-queue
            //        move ready context' from sleep-queue to ready-queue
            // no ready context, wait till signaled
            ready_queue_ev_.reset(
                (std::chrono::steady_clock::time_point::max)());
        }
        // push dispatcher context to ready-queue
        // so that ready-queue never becomes empty
        auto active_ctx = context::active();
        ready_queue_.push_back( * active_ctx);
        resume_( active_ctx, ctx);
            // FIXME: pump external event-loop like boost::asio::io_service
            //        react on external interrupt signals
            //        react on requestsin work sahring scenario
            // no ready context, wait till signaled
    }
    // interrupt all context' in ready- and sleep-queue
    release_terminated_();
    resume_( dispatcher_ctx_.get(), main_ctx_);
}

void
scheduler::set_ready( context * ctx) noexcept {
    BOOST_ASSERT( nullptr != ctx);
    BOOST_ASSERT( ! ctx->ready_is_linked() );
    BOOST_ASSERT( ! ctx->terminated_is_linked() );
    BOOST_ASSERT( ! ctx->wait_is_linked() );
    // set the scheduler for new fiber context
    ctx->set_scheduler( this);
    // push new fiber context to redy-queue
    ready_queue_.push_back( * ctx);
}

void
scheduler::set_terminated( context * ctx) noexcept {
    BOOST_ASSERT( nullptr != ctx);
    BOOST_ASSERT( ! ctx->ready_is_linked() );
    BOOST_ASSERT( ! ctx->terminated_is_linked() );
    BOOST_ASSERT( ! ctx->wait_is_linked() );
    terminated_queue_.push_back( * ctx);
}

void
scheduler::re_schedule( context * active_ctx) noexcept {
    BOOST_ASSERT( nullptr != active_ctx);
    resume_( active_ctx, get_next_() );
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
