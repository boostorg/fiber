
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/fiber/context.hpp"

#include "boost/fiber/exceptions.hpp"
#include "boost/fiber/interruption.hpp"
#include "boost/fiber/scheduler.hpp"

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

static context * make_main_context() {
    // main fiber context for this thread
    static thread_local context main_ctx( main_context);
    // scheduler for this thread
    static thread_local scheduler sched;
    // attach main fiber context to scheduler
    sched.set_main_context( & main_ctx);
    // create and attach dispatcher fiber context to scheduler
    sched.set_dispatcher_context(
        make_dispatcher_context( & sched) );
    return & main_ctx;
}

thread_local context *
context::active_ = make_main_context();

context *
context::active() noexcept {
    return active_;
}

void
context::active( context * active) noexcept {
    BOOST_ASSERT( nullptr != active);
    active_ = active;
}

void
context::set_terminated_() noexcept {
    // protect for concurrent access
    std::unique_lock< detail::spinlock > lk( splk_);
    flags_ |= flag_terminated;
    scheduler_->set_terminated( this);
}

// main fiber context
context::context( main_context_t) :
    use_count_( 1), // allocated on main- or thread-stack
    flags_( flag_main_context),
    scheduler_( nullptr),
    ctx_( boost::context::execution_context::current() ),
    worker_hook_(),
    terminated_hook_(),
    ready_hook_(),
    remote_ready_hook_(),
    sleep_hook_(),
    wait_hook_(),
    tp_( (std::chrono::steady_clock::time_point::max)() ),
    fss_data_(),
    wait_queue_(),
    splk_() {
}

// dispatcher fiber context
context::context( dispatcher_context_t, boost::context::preallocated const& palloc,
                  fixedsize_stack const& salloc, scheduler * sched) :
    use_count_( 0), // scheduler will own dispatcher context
    flags_( flag_dispatcher_context),
    scheduler_( nullptr),
    ctx_( palloc, salloc,
          [=] () -> void {
            // execute scheduler::dispatch()
            sched->dispatch();
            // dispatcher context should never return from scheduler::dispatch()
            BOOST_ASSERT_MSG( false, "disatcher fiber already terminated");
          }),
    worker_hook_(),
    terminated_hook_(),
    ready_hook_(),
    remote_ready_hook_(),
    sleep_hook_(),
    wait_hook_(),
    tp_( (std::chrono::steady_clock::time_point::max)() ),
    fss_data_(),
    wait_queue_(),
    splk_() {
}

context::~context() {
    BOOST_ASSERT( wait_queue_.empty() );
    BOOST_ASSERT( ! ready_is_linked() );
    BOOST_ASSERT( ! remote_ready_is_linked() );
    BOOST_ASSERT( ! sleep_is_linked() );
    BOOST_ASSERT( ! wait_is_linked() );
}

void
context::set_scheduler( scheduler * s) {
    BOOST_ASSERT( nullptr != s);
    scheduler_ = s;
}

scheduler *
context::get_scheduler() const noexcept {
    return scheduler_;
}

context::id
context::get_id() const noexcept {
    return id( const_cast< context * >( this) );
}

void
context::resume() {
    ctx_();
}

void
context::suspend() noexcept {
    scheduler_->re_schedule( this);
}

void
context::release() noexcept {
    BOOST_ASSERT( is_terminated() );

    wait_queue_t tmp;
    // protect for concurrent access
    std::unique_lock< detail::spinlock > lk( splk_);
    tmp.swap( wait_queue_);
    lk.unlock();
    // notify all waiting fibers
    wait_queue_t::iterator e = tmp.end();
    for ( wait_queue_t::iterator i = tmp.begin(); i != e;) {
        context * ctx = & ( * i);
        // remove fiber from wait-queue
        i = tmp.erase( i);
        // notify scheduler
        scheduler_->set_ready( ctx);
    }
    // release fiber-specific-data
    for ( fss_data_t::value_type & data : fss_data_) {
        data.second.do_cleanup();
    }
    fss_data_.clear();
}

void
context::join() {
    // get active context
    context * active_ctx = context::active();
    // context::join() is a interruption point
    this_fiber::interruption_point();
    // protect for concurrent access
    std::unique_lock< detail::spinlock > lk( splk_);
    // wait for context which is not terminated
    if ( 0 == ( flags_ & flag_terminated) ) {
        // push active context to wait-queue, member
        // of the context which has to be joined by
        // the active context
        wait_queue_.push_back( * active_ctx);
        lk.unlock();
        // suspend active context
        scheduler_->re_schedule( active_ctx);
        // remove from wait-queue
        active_ctx->wait_unlink();
        // active context resumed
        BOOST_ASSERT( context::active() == active_ctx);
    }
    // context::join() is a interruption point
    this_fiber::interruption_point();
}

void
context::yield() noexcept {
    // get active context
    context * active_ctx = context::active();
    // yield active context
    scheduler_->yield( active_ctx);
}

bool
context::wait_until( std::chrono::steady_clock::time_point const& tp) noexcept {
    BOOST_ASSERT( nullptr != scheduler_);
    BOOST_ASSERT( this == active_);

    return scheduler_->wait_until( this, tp);
}

void
context::set_ready( context * ctx) noexcept {
    BOOST_ASSERT( nullptr != ctx);
    BOOST_ASSERT( this != ctx);
    BOOST_ASSERT( nullptr != scheduler_);
    BOOST_ASSERT( nullptr != ctx->scheduler_);
    // FIXME: comparing scheduler address' must be synchronized?
    //        what if ctx is migrated between threads
    //        (other scheduler assigned)
    if ( scheduler_ == ctx->scheduler_) {
        // local
        scheduler_->set_ready( ctx);
    } else {
        // remote
        ctx->scheduler_->set_remote_ready( ctx);
    }
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
    BOOST_ASSERT( ! is_main_context() && ! is_dispatcher_context() );
    if ( req) {
        flags_ |= flag_interruption_requested;
    } else {
        flags_ &= ~flag_interruption_requested;
    }
}

void
context::request_unwinding() noexcept {
    BOOST_ASSERT( ! is_main_context() );
    BOOST_ASSERT( ! is_dispatcher_context() );
    flags_ |= flag_forced_unwind;
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

bool
context::managed_is_linked() {
    return worker_hook_.is_linked();
}

bool
context::ready_is_linked() {
    return ready_hook_.is_linked();
}

bool
context::remote_ready_is_linked() {
    return remote_ready_hook_.is_linked();
}

bool
context::sleep_is_linked() {
    return sleep_hook_.is_linked();
}

bool
context::wait_is_linked() {
    return wait_hook_.is_linked();
}

void
context::managed_unlink() {
    worker_hook_.unlink();
}

void
context::sleep_unlink() {
    sleep_hook_.unlink();
}

void
context::wait_unlink() {
    wait_hook_.unlink();
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
