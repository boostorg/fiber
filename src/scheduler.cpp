
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/fiber/scheduler.hpp"

#include <chrono>
#include <mutex>

#include <boost/assert.hpp>

#include "boost/fiber/context.hpp"
#include "boost/fiber/exceptions.hpp"
#include "boost/fiber/round_robin.hpp"

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

void
scheduler::resume_( context * active_ctx, context * ctx) {
    BOOST_ASSERT( nullptr != active_ctx);
    BOOST_ASSERT( nullptr != ctx);
    BOOST_ASSERT( main_ctx_ == active_ctx ||
                  dispatcher_ctx_.get() == active_ctx ||
                  active_ctx->worker_is_linked() );
    BOOST_ASSERT( main_ctx_ == ctx ||
                  dispatcher_ctx_.get() == ctx ||
                  ctx->worker_is_linked() );
    BOOST_ASSERT( this == active_ctx->get_scheduler() );
    BOOST_ASSERT( this == ctx->get_scheduler() );
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
    BOOST_ASSERT( context::active() == active_ctx);
    BOOST_ASSERT( main_ctx_ == active_ctx ||
                  dispatcher_ctx_.get() == active_ctx ||
                 active_ctx->worker_is_linked() );
    // check if unwinding was requested
    if ( active_ctx->unwinding_requested() ) {
        throw forced_unwind();
    }
}

context *
scheduler::get_next_() noexcept {
    context * ctx = sched_algo_->pick_next();
    if ( nullptr != ctx &&
         ! ctx->worker_is_linked() &&
         ! ctx->is_main_context() &&
         ! ctx->is_dispatcher_context() ) {
        ctx->worker_link( worker_queue_);
    }
    return ctx;
}

void
scheduler::release_terminated_() {
    terminated_queue_t::iterator e( terminated_queue_.end() );
    for ( terminated_queue_t::iterator i( terminated_queue_.begin() );
            i != e;) {
        context * ctx = & ( * i);
        BOOST_ASSERT( ! ctx->is_main_context() );
        BOOST_ASSERT( ! ctx->is_dispatcher_context() );
        BOOST_ASSERT( ctx->worker_is_linked() );
        BOOST_ASSERT( ctx->is_terminated() );
        BOOST_ASSERT( ! ctx->ready_is_linked() );
        BOOST_ASSERT( ! ctx->sleep_is_linked() );
        i = terminated_queue_.erase( i);
        // if last reference, e.g. fiber::join() or fiber::detach()
        // have been already called, this will call ~context(),
        // the context is automatically removeid from worker-queue
        intrusive_ptr_release( ctx);
    }
}

void
scheduler::remote_ready2ready_() {
    remote_ready_queue_t tmp;
    // protect for concurrent access
    std::unique_lock< detail::spinlock > lk( remote_ready_splk_);
    remote_ready_queue_.swap( tmp);
    lk.unlock();
    // get context from remote ready-queue
    while ( ! tmp.empty() ) {
        context * ctx = & tmp.front();
        tmp.pop_front();
        // store context in local queues
        set_ready( ctx);
    }
}

void
scheduler::yield2ready_() {
    // get context from yield-queue
    while ( ! yield_queue_.empty() ) {
        context * ctx = & yield_queue_.front();
        yield_queue_.pop_front();
        BOOST_ASSERT( ! ctx->ready_is_linked() );
        set_ready( ctx);
    }
}

void
scheduler::sleep2ready_() noexcept {
    // move context which the deadline has reached
    // to ready-queue
    // sleep-queue is sorted (ascending)
    std::chrono::steady_clock::time_point now =
        std::chrono::steady_clock::now();
    sleep_queue_t::iterator e = sleep_queue_.end();
    for ( sleep_queue_t::iterator i = sleep_queue_.begin(); i != e;) {
        context * ctx = & ( * i);
        BOOST_ASSERT( ! ctx->is_dispatcher_context() );
        BOOST_ASSERT( main_ctx_ == ctx ||
                      ctx->worker_is_linked() );
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
            sched_algo_->awakened( ctx);
        } else {
            break; // first context with now < deadline
        }
    }
}

scheduler::scheduler() noexcept :
    sched_algo_( new round_robin() ),
    main_ctx_( nullptr),
    dispatcher_ctx_(),
    worker_queue_(),
    terminated_queue_(),
    remote_ready_queue_(),
    yield_queue_(),
    sleep_queue_(),
    shutdown_( false),
    remote_ready_splk_() {
}

scheduler::~scheduler() noexcept {
    BOOST_ASSERT( nullptr != main_ctx_);
    BOOST_ASSERT( nullptr != dispatcher_ctx_.get() );
    BOOST_ASSERT( context::active() == main_ctx_);
    // signal dispatcher-context termination
    shutdown_ = true;
    // resume pending fibers
    resume_( main_ctx_, get_next_() );
    // no context' in worker-queue
    BOOST_ASSERT( worker_queue_.empty() );
    BOOST_ASSERT( terminated_queue_.empty() );
    BOOST_ASSERT( ! sched_algo_->has_ready_fibers() );
    BOOST_ASSERT( remote_ready_queue_.empty() );
    BOOST_ASSERT( yield_queue_.empty() );
    BOOST_ASSERT( sleep_queue_.empty() );
    // set active context to nullptr
    context::reset_active();
    // deallocate dispatcher-context
    dispatcher_ctx_.reset();
    // set main-context to nullptr
    main_ctx_ = nullptr;
}

void
scheduler::set_main_context( context * main_ctx) noexcept {
    BOOST_ASSERT( nullptr != main_ctx);
    // main-context represents the execution context created
    // by the system, e.g. main()- or thread-context
    // should not be in worker-queue
    main_ctx_ = main_ctx;
    main_ctx_->set_scheduler( this);
}

void
scheduler::set_dispatcher_context( intrusive_ptr< context > dispatcher_ctx) noexcept {
    BOOST_ASSERT( dispatcher_ctx);
    // dispatcher context has to handle
    //    - remote ready context'
    //    - sleeping context'
    //    - extern event-loops
    //    - suspending the thread if ready-queue is empty (waiting on external event)
    // should not be in worker-queue
    dispatcher_ctx_.swap( dispatcher_ctx);
    // add dispatcher-context to ready-queue
    // so it is the first element in the ready-queue
    // if the main context tries to suspend the first time
    // the dispatcher-context is resumed and
    // scheduler::dispatch() is executed
    dispatcher_ctx_->set_scheduler( this);
    sched_algo_->awakened( dispatcher_ctx_.get() );
}

void
scheduler::dispatch() {
    BOOST_ASSERT( context::active() == dispatcher_ctx_);
    while ( ! shutdown_) {
        // move yielded context' to ready-queue
        yield2ready_();
        // release termianted context'
        release_terminated_();
        // get sleeping context'
        sleep2ready_();
        // get context' from remote ready-queue
        remote_ready2ready_();
        // FIXME: local and remote ready-queue contain same context
        context * ctx = nullptr;
        // loop till we get next ready context
        while ( nullptr == ( ctx = get_next_() ) ) {
            // get context' from remote ready-queue
            remote_ready2ready_();
            // no ready context, wait till signaled
            // set deadline to highest value
            std::chrono::steady_clock::time_point suspend_time =
                    (std::chrono::steady_clock::time_point::max)();
            // get lowest deadline from sleep-queue
            sleep_queue_t::iterator i = sleep_queue_.begin();
            if ( sleep_queue_.end() != i) {
                suspend_time = i->tp_;
            }
            // no ready context, wait till signaled
            sched_algo_->suspend_until( suspend_time);
            // get sleeping context'
            sleep2ready_();
            // TODO: pump external event-loop like boost::asio::io_service
            //       react on external interrupt signals
            //       react on requesting work sharing scenario
            //       no ready context, wait till signaled
        }
        // push dispatcher-context to ready-queue
        // so that ready-queue never becomes empty
        sched_algo_->awakened( dispatcher_ctx_.get() );
        resume_( dispatcher_ctx_.get(), ctx);
        BOOST_ASSERT( context::active() == dispatcher_ctx_.get() );
    }
    // loop till all context' have been terminated
    while ( ! worker_queue_.empty() ) {
        release_terminated_();
        // force unwinding of all context' in worker-queue
        worker_queue_t::iterator e = worker_queue_.end();
        for ( worker_queue_t::iterator i = worker_queue_.begin(); i != e; ++i) {
            context * ctx = & ( * i);
            ctx->request_unwinding();
            set_ready( ctx); 
        }
    }
    resume_( dispatcher_ctx_.get(), main_ctx_);
}

void
scheduler::set_ready( context * ctx) noexcept {
    BOOST_ASSERT( nullptr != ctx);
    BOOST_ASSERT( ! ctx->is_terminated() );
    // dispatcher-context will never be passed to set_ready()
    BOOST_ASSERT( ! ctx->is_dispatcher_context() );
    // we do not test for wait-queue because
    // context::wait_is_linked() is not synchronized
    // with other threads
    //BOOST_ASSERT( active_ctx->wait_is_linked() );
    // handle newly created context
    if ( ! ctx->is_main_context() ) {
        if ( ! ctx->worker_is_linked() ) {
            // attach context to `this`-scheduler
            ctx->set_scheduler( this);
            // push to the worker-queue
            ctx->worker_link( worker_queue_);
        }
    } else {
        // sanity checks, main-context might by signaled
        // from another thread
        BOOST_ASSERT( main_ctx_ == ctx);
        BOOST_ASSERT( this == ctx->get_scheduler() );
    }
    // remove context ctx from sleep-queue
    // (might happen if blocked in timed_mutex::try_lock_until())
    // FIXME: mabye better done in scheduler::dispatch()
    if ( ctx->sleep_is_linked() ) {
        // unlink it from sleep-queue
        ctx->sleep_unlink();
    }
    // for safety unlink it from ready-queue
    // this might happen if a newly created fiber was
    // signaled to interrupt
    ctx->ready_unlink();
    // push new context to ready-queue
    sched_algo_->awakened( ctx);
}

void
scheduler::set_remote_ready( context * ctx) noexcept {
    BOOST_ASSERT( nullptr != ctx);
    BOOST_ASSERT( ! ctx->is_dispatcher_context() );
    BOOST_ASSERT( this == ctx->get_scheduler() );
    // another thread might signal the main-context
    // from this thread
    //BOOST_ASSERT( ! ctx->is_main_context() );
    // context ctx might in wait-/ready-/sleep-queue
    // we do not test this in this function
    // scheduler::dispatcher() has to take care
    // protect for concurrent access
    std::unique_lock< detail::spinlock > lk( remote_ready_splk_);
    // push new context to remote ready-queue
    ctx->remote_ready_link( remote_ready_queue_);
    lk.unlock();
    // notify scheduler
    sched_algo_->notify();
}

void
scheduler::set_terminated( context * ctx) noexcept {
    BOOST_ASSERT( nullptr != ctx);
    BOOST_ASSERT( ! ctx->is_main_context() );
    BOOST_ASSERT( ! ctx->is_dispatcher_context() );
    BOOST_ASSERT( ctx->worker_is_linked() );
    BOOST_ASSERT( ctx->is_terminated() );
    BOOST_ASSERT( ! ctx->ready_is_linked() );
    BOOST_ASSERT( ! ctx->sleep_is_linked() );
    BOOST_ASSERT( ! ctx->wait_is_linked() );
    // store the terminated fiber in the terminated-queue
    // the dispatcher-context will call 
    // intrusive_ptr_release( ctx);
    ctx->terminated_link( terminated_queue_);
}

void
scheduler::yield( context * active_ctx) noexcept {
    BOOST_ASSERT( nullptr != active_ctx);
    BOOST_ASSERT( main_ctx_ == active_ctx ||
                  dispatcher_ctx_.get() == active_ctx ||
                  active_ctx->worker_is_linked() );
    BOOST_ASSERT( ! active_ctx->is_terminated() );
    BOOST_ASSERT( ! active_ctx->ready_is_linked() );
    BOOST_ASSERT( ! active_ctx->sleep_is_linked() );
    // we do not test for wait-queue because
    // context::wait_is_linked() is not sychronized
    // with other threads
    // push active context to yield-queue
    // in work-sharing context (multiple threads read
    // from one ready-queue) the context must be
    // already suspended until another thread resumes it
    active_ctx->yield_link( yield_queue_);
    // resume another fiber
    resume_( active_ctx, get_next_() );
}

bool
scheduler::wait_until( context * active_ctx,
                       std::chrono::steady_clock::time_point const& sleep_tp) noexcept {
    BOOST_ASSERT( nullptr != active_ctx);
    BOOST_ASSERT( main_ctx_ == active_ctx ||
                  dispatcher_ctx_.get() == active_ctx ||
                  active_ctx->worker_is_linked() );
    BOOST_ASSERT( ! active_ctx->is_terminated() );
    // if the active-fiber running in this thread calls
    // condition:wait() and code in another thread calls
    // condition::notify_one(), it might happen that the
    // other thread pushes the fiber to remote ready-queue first
    // the dispatcher-context migh have been moved the fiber from
    // the remote ready-queue to the local ready-queue
    // so we do not check
    //BOOST_ASSERT( active_ctx->ready_is_linked() );
    BOOST_ASSERT( ! active_ctx->sleep_is_linked() );
    // active_ctx->wait_is_linked() might return true
    // if context was locked inside timed_mutex::try_lock_until()
    // context::wait_is_linked() is not sychronized
    // with other threads
    // push active context to sleep-queue
    active_ctx->tp_ = sleep_tp;
    active_ctx->sleep_link( sleep_queue_);
    // resume another context
    resume_( active_ctx, get_next_() );
    // context has been resumed
    // check if deadline has reached
    return std::chrono::steady_clock::now() < sleep_tp;
}

void
scheduler::re_schedule( context * active_ctx) noexcept {
    BOOST_ASSERT( nullptr != active_ctx);
    BOOST_ASSERT( main_ctx_ == active_ctx ||
                  dispatcher_ctx_.get() == active_ctx ||
                  active_ctx->worker_is_linked() );
    // resume another context
    resume_( active_ctx, get_next_() );
}

bool
scheduler::has_ready_fibers() const noexcept {
    return sched_algo_->has_ready_fibers();
}

void
scheduler::set_sched_algo( std::unique_ptr< sched_algorithm > algo) {
    // move remaining cotnext in current scheduler to new one
    while ( sched_algo_->has_ready_fibers() ) {
        algo->awakened( sched_algo_->pick_next() );
    }
    sched_algo_ = std::move( algo);
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
