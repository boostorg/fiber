
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/fiber/scheduler.hpp"

#include <chrono>
#include <mutex>                    // std::unique_lock

#include <boost/assert.hpp>

#include "boost/fiber/context.hpp"
#include "boost/fiber/exceptions.hpp"
#include "boost/fiber/round_robin.hpp"

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

context *
scheduler::get_next_() noexcept {
    context * ctx = sched_algo_->pick_next();
    //BOOST_ASSERT( nullptr == ctx);
    //BOOST_ASSERT( this == ctx->get_scheduler() );
    return ctx;
}

void
scheduler::release_terminated_() noexcept {
    terminated_queue_t::iterator e( terminated_queue_.end() );
    for ( terminated_queue_t::iterator i( terminated_queue_.begin() );
            i != e;) {
        context * ctx = & ( * i);
        BOOST_ASSERT( ! ctx->is_context( type::main_context) );
        BOOST_ASSERT( ! ctx->is_context( type::dispatcher_context) );
        //BOOST_ASSERT( ctx->worker_is_linked() );
        BOOST_ASSERT( ctx->is_terminated() );
        BOOST_ASSERT( ! ctx->ready_is_linked() );
        BOOST_ASSERT( ! ctx->sleep_is_linked() );
        // remove context from worker-queue
        std::unique_lock< detail::spinlock > lk( worker_splk_);
        ctx->worker_unlink();
        lk.unlock();
        // remove context from terminated-queue
        i = terminated_queue_.erase( i);
        // if last reference, e.g. fiber::join() or fiber::detach()
        // have been already called, this will call ~context(),
        // the context is automatically removeid from worker-queue
        intrusive_ptr_release( ctx);
    }
}

void
scheduler::remote_ready2ready_() noexcept {
    // protect for concurrent access
    std::unique_lock< detail::spinlock > lk( remote_ready_splk_);
    // get context from remote ready-queue
    while ( ! remote_ready_queue_.empty() ) {
        context * ctx = & remote_ready_queue_.front();
        remote_ready_queue_.pop_front();
        // store context in local queues
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
        BOOST_ASSERT( ! ctx->is_context( type::dispatcher_context) );
        //BOOST_ASSERT( main_ctx_ == ctx || ctx->worker_is_linked() );
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
    sched_algo_{ new round_robin() } {
}

scheduler::~scheduler() {
    BOOST_ASSERT( nullptr != main_ctx_);
    BOOST_ASSERT( nullptr != dispatcher_ctx_.get() );
    BOOST_ASSERT( context::active() == main_ctx_);
    // signal dispatcher-context termination
    shutdown_ = true;
    // resume pending fibers
    get_next_()->resume();
    // no context' in worker-queue
    //BOOST_ASSERT( worker_queue_.empty() );
    BOOST_ASSERT( terminated_queue_.empty() );
    BOOST_ASSERT( ! sched_algo_->has_ready_fibers() );
    BOOST_ASSERT( remote_ready_queue_.empty() );
    BOOST_ASSERT( sleep_queue_.empty() );
    // set active context to nullptr
    context::reset_active();
    // deallocate dispatcher-context
    dispatcher_ctx_.reset();
    // set main-context to nullptr
    main_ctx_ = nullptr;
}

#if (BOOST_EXECUTION_CONTEXT==1)
void
scheduler::dispatch() noexcept {
#else
boost::context::execution_context< detail::data_t * >
scheduler::dispatch() noexcept {
#endif
    BOOST_ASSERT( context::active() == dispatcher_ctx_);
    while ( ! shutdown_) {
        // release termianted context'
        release_terminated_();
        // get context' from remote ready-queue
        remote_ready2ready_();
        // get sleeping context'
        sleep2ready_();
        // get next ready context
        context * ctx = get_next_();
        if ( nullptr != ctx) {
            // push dispatcher-context to ready-queue
            // so that ready-queue never becomes empty
            sched_algo_->awakened( dispatcher_ctx_.get() );
            ctx->resume();
            BOOST_ASSERT( context::active() == dispatcher_ctx_.get() );
        } else {
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
        }
    }
    // loop till all context' have been terminated
    std::unique_lock< detail::spinlock > lk( worker_splk_);
    while ( ! worker_queue_.empty() ) {
        // interrupt all context' in worker-queue
        worker_queue_t::iterator e = worker_queue_.end();
        for ( worker_queue_t::iterator i = worker_queue_.begin(); i != e;) {
            context * ctx = & ( worker_queue_.front() );
            BOOST_ASSERT( ! ctx->is_context( type::main_context) );
            BOOST_ASSERT( ! ctx->is_context( type::dispatcher_context) );
            if ( ctx->is_terminated() ) {
                i = worker_queue_.erase( i);
            } else {
                ctx->request_interruption( true);
                set_ready( ctx);
                ++i;
            }
        }
        context * ctx = nullptr;
        if ( nullptr != ( ctx = get_next_() ) ) {
            // resume ready context's
            sched_algo_->awakened( dispatcher_ctx_.get() );
            ctx->resume();
            BOOST_ASSERT( context::active() == dispatcher_ctx_.get() );
        }
    }
    lk.unlock();
    // release termianted context'
    release_terminated_();
    // return to main-context
#if (BOOST_EXECUTION_CONTEXT==1)
    main_ctx_->resume();
#else
    return main_ctx_->suspend_with_cc();
#endif
}

void
scheduler::set_ready( context * ctx) noexcept {
    BOOST_ASSERT( nullptr != ctx);
    BOOST_ASSERT( ! ctx->is_terminated() );
    // dispatcher-context will never be passed to set_ready()
    BOOST_ASSERT( ! ctx->is_context( type::dispatcher_context) );
    // we do not test for wait-queue because
    // context::wait_is_linked() is not synchronized
    // with other threads
    //BOOST_ASSERT( active_ctx->wait_is_linked() );
    // remove context ctx from sleep-queue
    // (might happen if blocked in timed_mutex::try_lock_until())
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
    BOOST_ASSERT( ! ctx->is_context( type::dispatcher_context) );
    BOOST_ASSERT( this == ctx->get_scheduler() );
    // another thread might signal the main-context
    // from this thread
    //BOOST_ASSERT( ! ctx->is_context( type::main_context) );
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

#if (BOOST_EXECUTION_CONTEXT==1)
void
scheduler::set_terminated( context * active_ctx) noexcept {
#else
boost::context::execution_context< detail::data_t * >
scheduler::set_terminated( context * active_ctx) noexcept {
#endif
    BOOST_ASSERT( nullptr != active_ctx);
    BOOST_ASSERT( context::active() == active_ctx);
    BOOST_ASSERT( ! active_ctx->is_context( type::main_context) );
    BOOST_ASSERT( ! active_ctx->is_context( type::dispatcher_context) );
    //BOOST_ASSERT( active_ctx->worker_is_linked() );
    BOOST_ASSERT( active_ctx->is_terminated() );
    BOOST_ASSERT( ! active_ctx->ready_is_linked() );
    BOOST_ASSERT( ! active_ctx->sleep_is_linked() );
    BOOST_ASSERT( ! active_ctx->wait_is_linked() );
    // store the terminated fiber in the terminated-queue
    // the dispatcher-context will call 
    // intrusive_ptr_release( ctx);
    active_ctx->terminated_link( terminated_queue_);
    // resume another fiber
#if (BOOST_EXECUTION_CONTEXT==1)
    get_next_()->resume();
#else
    return get_next_()->suspend_with_cc();
#endif
}

void
scheduler::yield( context * active_ctx) noexcept {
    BOOST_ASSERT( nullptr != active_ctx);
    //BOOST_ASSERT( main_ctx_ == active_ctx || dispatcher_ctx_.get() == active_ctx || active_ctx->worker_is_linked() );
    BOOST_ASSERT( ! active_ctx->is_terminated() );
    BOOST_ASSERT( ! active_ctx->ready_is_linked() );
    BOOST_ASSERT( ! active_ctx->sleep_is_linked() );
    // we do not test for wait-queue because
    // context::wait_is_linked() is not sychronized
    // with other threads
    // defer passing active context to set_ready()
    // in work-sharing context (multiple threads read
    // from one ready-queue) the context must be
    // already suspended until another thread resumes it
    // (== maked as ready)
    // resume another fiber
    get_next_()->resume( active_ctx);
}

bool
scheduler::wait_until( context * active_ctx,
                       std::chrono::steady_clock::time_point const& sleep_tp) noexcept {
    BOOST_ASSERT( nullptr != active_ctx);
    //BOOST_ASSERT( main_ctx_ == active_ctx || dispatcher_ctx_.get() == active_ctx || active_ctx->worker_is_linked() );
    BOOST_ASSERT( ! active_ctx->is_terminated() );
    // if the active-fiber running in this thread calls
    // condition_variable:wait() and code in another thread calls
    // condition_variable::notify_one(), it might happen that the
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
    get_next_()->resume();
    // context has been resumed
    // check if deadline has reached
    return std::chrono::steady_clock::now() < sleep_tp;
}

bool
scheduler::wait_until( context * active_ctx,
                       std::chrono::steady_clock::time_point const& sleep_tp,
                       detail::spinlock_lock & lk) noexcept {
    BOOST_ASSERT( nullptr != active_ctx);
    //BOOST_ASSERT( main_ctx_ == active_ctx || dispatcher_ctx_.get() == active_ctx || active_ctx->worker_is_linked() );
    BOOST_ASSERT( ! active_ctx->is_terminated() );
    // if the active-fiber running in this thread calls
    // condition_variable:wait() and code in another thread calls
    // condition_variable::notify_one(), it might happen that the
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
    get_next_()->resume( lk);
    // context has been resumed
    // check if deadline has reached
    return std::chrono::steady_clock::now() < sleep_tp;
}

void
scheduler::suspend( context * active_ctx) noexcept {
    BOOST_ASSERT( nullptr != active_ctx);
    //BOOST_ASSERT( main_ctx_ == active_ctx || dispatcher_ctx_.get() == active_ctx || active_ctx->worker_is_linked() );
    // resume another context
    get_next_()->resume();
}

void
scheduler::suspend( context * active_ctx,
                    detail::spinlock_lock & lk) noexcept {
    BOOST_ASSERT( nullptr != active_ctx);
    //BOOST_ASSERT( main_ctx_ == active_ctx || dispatcher_ctx_.get() == active_ctx || active_ctx->worker_is_linked() );
    // resume another context
    get_next_()->resume( lk);
}

bool
scheduler::has_ready_fibers() const noexcept {
    return sched_algo_->has_ready_fibers();
}

void
scheduler::set_sched_algo( std::unique_ptr< sched_algorithm > algo) noexcept {
    // move remaining cotnext in current scheduler to new one
    while ( sched_algo_->has_ready_fibers() ) {
        algo->awakened( sched_algo_->pick_next() );
    }
    sched_algo_ = std::move( algo);
}

void
scheduler::attach_main_context( context * main_ctx) noexcept {
    BOOST_ASSERT( nullptr != main_ctx);
    // main-context represents the execution context created
    // by the system, e.g. main()- or thread-context
    // should not be in worker-queue
    main_ctx_ = main_ctx;
    main_ctx_->scheduler_ = this;
}

void
scheduler::attach_dispatcher_context( intrusive_ptr< context > dispatcher_ctx) noexcept {
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
    dispatcher_ctx_->scheduler_ = this;
    sched_algo_->awakened( dispatcher_ctx_.get() );
}

void
scheduler::attach_worker_context( context * ctx) noexcept {
    BOOST_ASSERT( nullptr != ctx);
    BOOST_ASSERT( ! ctx->ready_is_linked() );
    BOOST_ASSERT( ! ctx->remote_ready_is_linked() );
    BOOST_ASSERT( ! ctx->sleep_is_linked() );
    BOOST_ASSERT( ! ctx->terminated_is_linked() );
    BOOST_ASSERT( ! ctx->wait_is_linked() );
    std::unique_lock< detail::spinlock > lk( worker_splk_);
    BOOST_ASSERT( ! ctx->worker_is_linked() );
    ctx->worker_link( worker_queue_);
    ctx->scheduler_ = this;
}

void
scheduler::detach_worker_context( context * ctx) noexcept {
    BOOST_ASSERT( nullptr != ctx);
    BOOST_ASSERT( ! ctx->ready_is_linked() );
    BOOST_ASSERT( ! ctx->remote_ready_is_linked() );
    BOOST_ASSERT( ! ctx->sleep_is_linked() );
    BOOST_ASSERT( ! ctx->terminated_is_linked() );
    BOOST_ASSERT( ! ctx->wait_is_linked() );
    BOOST_ASSERT( ! ctx->wait_is_linked() );
    std::unique_lock< detail::spinlock > lk( worker_splk_);
    ctx->worker_unlink();
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
