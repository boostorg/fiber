
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/fiber/scheduler.hpp"

#include <chrono>
#include <mutex>

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

void
scheduler::move_from_remote_() {
    // protect for concurrent access
    std::unique_lock< detail::spinlock > lk( remote_ready_splk_);
    // get from remote ready-queue
    remote_ready_queue_t::iterator e = remote_ready_queue_.end();
    for ( remote_ready_queue_t::iterator i = remote_ready_queue_.begin(); i != e;) {
        context * ctx = & ( * i);
        i = remote_ready_queue_.erase( i);
        // context must no be contained in ready-queue
        // no need to check agains active context
        // because we are in scheduler::dispatch() -> dispatcher-context
        if ( ! ctx->ready_is_linked() ) {
            ready_queue_.push_back( * ctx);
        }
    }
}

void
scheduler::woken_up_() noexcept {
    // move context which the deadline has reached
    // to ready-queue
    // sleep-queue is sorted (ascending)
    std::chrono::steady_clock::time_point now =
        std::chrono::steady_clock::now();
    sleep_queue_t::iterator e = sleep_queue_.end();
    for ( sleep_queue_t::iterator i = sleep_queue_.begin(); i != e;) {
        context * ctx = & ( * i);
        BOOST_ASSERT( ! ctx->is_terminated() );
        BOOST_ASSERT( ! ctx->ready_is_linked() );
        BOOST_ASSERT( ctx->sleep_is_linked() );
        // ctx->wait_is_linked() might return true if
        // context is waiting in time_mutex::try_lock_until()
        // set fiber to state_ready if deadline was reached
        if ( ctx->tp_ <= now) {
            // remove context from sleep-queue
            i = sleep_queue_.erase( i);
            // reset sleep-tp
            ctx->tp_ = (std::chrono::steady_clock::time_point::max)();
            // push new context to ready-queue
            ready_queue_.push_back( * ctx);
        } else {
            break; // first element with ctx->tp_ > now, leave for-loop
        }
    }
}

scheduler::scheduler() noexcept :
    main_ctx_( nullptr),
    dispatcher_ctx_(),
    ready_queue_(),
    remote_ready_queue_(),
    terminated_queue_(),
    shutdown_( false),
    ready_queue_ev_(),
    remote_ready_splk_() {
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
        // release termianted context'
        release_terminated_();
        // get sleeping context'
        woken_up_();
        // get context' from remote ready-queue
        move_from_remote_();
        context * ctx = nullptr;
        // loop till we get next ready context
        while ( nullptr == ( ctx = get_next_() ) ) {
            // get context' from remote ready-queue
            move_from_remote_();
            // no ready context, wait till signaled
            // set deadline to highest value
            std::chrono::steady_clock::time_point tp =
                    (std::chrono::steady_clock::time_point::max)();
            // get lowest deadline from sleep-queue
            sleep_queue_t::iterator i = sleep_queue_.begin();
            if ( sleep_queue_.end() != i) {
                tp = i->tp_;
            }
            ready_queue_ev_.reset( tp);
            // get sleeping context'
            woken_up_();
        }
        // push dispatcher context to ready-queue
        // so that ready-queue never becomes empty
        auto active_ctx = context::active();
        ready_queue_.push_back( * active_ctx);
        resume_( active_ctx, ctx);
            // TODO: pump external event-loop like boost::asio::io_service
            //       react on external interrupt signals
            //       react on requestsin work sahring scenario
            // no ready context, wait till signaled
    }
    // interrupt all context' in ready- and sleep-queue
    release_terminated_();
    resume_( dispatcher_ctx_.get(), main_ctx_);
}

void
scheduler::set_ready( context * ctx) noexcept {
    BOOST_ASSERT( nullptr != ctx);
    BOOST_ASSERT( ! ctx->is_terminated() );
    BOOST_ASSERT( ! ctx->ready_is_linked() );
    // we do not test for wait-queue because
    // context::wait_is_linked() is not sychronized
    // with other threads
    // remove context ctx from sleep-queue
    // (might happen if blocked in timed_mutex::try_lock_until())
    if ( ctx->sleep_is_linked() ) {
        // unlink it from sleep-queue
        ctx->sleep_unlink();
    }
    // set the scheduler for new context
    ctx->set_scheduler( this);
    // push new context to ready-queue
    ready_queue_.push_back( * ctx);
}

void
scheduler::set_remote_ready( context * ctx) noexcept {
    BOOST_ASSERT( nullptr != ctx);
    // context ctx might in wait-/ready-/sleep-queue
    // we do not test this in this function
    // scheduler::dispatcher() has to take care
    ctx->set_scheduler( this);
    // protect for concurrent access
    std::unique_lock< detail::spinlock > lk( remote_ready_splk_);
    // push new context to remote ready-queue
    remote_ready_queue_.push_back( * ctx);
}

void
scheduler::set_terminated( context * ctx) noexcept {
    BOOST_ASSERT( nullptr != ctx);
    BOOST_ASSERT( ctx->is_terminated() );
    BOOST_ASSERT( ! ctx->ready_is_linked() );
    BOOST_ASSERT( ! ctx->sleep_is_linked() );
    BOOST_ASSERT( ! ctx->wait_is_linked() );
    terminated_queue_.push_back( * ctx);
}

void
scheduler::yield( context * active_ctx) noexcept {
    BOOST_ASSERT( nullptr != active_ctx);
    BOOST_ASSERT( ! active_ctx->is_terminated() );
    BOOST_ASSERT( ! active_ctx->ready_is_linked() );
    BOOST_ASSERT( ! active_ctx->sleep_is_linked() );
    // we do not test for wait-queue because
    // context::wait_is_linked() is not sychronized
    // with other threads
    // push active context to ready-queue
    ready_queue_.push_back( * active_ctx);
    // resume another fiber
    resume_( active_ctx, get_next_() );
}

bool
scheduler::wait_until( context * active_ctx,
                       std::chrono::steady_clock::time_point const& sleep_tp) noexcept {
    BOOST_ASSERT( nullptr != active_ctx);
    BOOST_ASSERT( ! active_ctx->is_terminated() );
    BOOST_ASSERT( ! active_ctx->ready_is_linked() );
    BOOST_ASSERT( ! active_ctx->sleep_is_linked() );
    // active_ctx->wait_is_linked() might return true
    // if context was locked inside timed_mutex::try_lock_until()
    // context::wait_is_linked() is not sychronized
    // with other threads
    // push active context to sleep-queue
    active_ctx->tp_ = sleep_tp;
    sleep_queue_.insert( * active_ctx);
    // resume another context
    resume_( active_ctx, get_next_() );
    // context has been resumed
    // check if deadline has reached
    return std::chrono::steady_clock::now() < sleep_tp;
}

void
scheduler::re_schedule( context * active_ctx) noexcept {
    BOOST_ASSERT( nullptr != active_ctx);
    // resume another context
    resume_( active_ctx, get_next_() );
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
