
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/fiber/context.hpp"

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
    flags_ |= flag_terminated;
    scheduler_->set_terminated( this);
}

void
context::suspend_() noexcept {
    scheduler_->re_schedule( this);
}

// main fiber context
context::context( main_context_t) :
    ready_hook_(),
    terminated_hook_(),
    wait_hook_(),
    use_count_( 1), // allocated on main- or thread-stack
    flags_( flag_main_context),
    scheduler_( nullptr),
    ctx_( boost::context::execution_context::current() ),
    wait_queue_() {
}

// dispatcher fiber context
context::context( dispatcher_context_t, boost::context::preallocated const& palloc,
                  fixedsize_stack const& salloc, scheduler * sched) :
    ready_hook_(),
    terminated_hook_(),
    wait_hook_(),
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
    wait_queue_() {
}

context::~context() {
    BOOST_ASSERT( ! wait_is_linked() );
    BOOST_ASSERT( wait_queue_.empty() );
    BOOST_ASSERT( ! ready_is_linked() );
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
context::release() noexcept {
    BOOST_ASSERT( is_terminated() );

    // notify all waiting fibers
    wait_queue_t::iterator e = wait_queue_.end();
    for ( wait_queue_t::iterator i = wait_queue_.begin(); i != e;) {
        context * f = & ( * i);
        // remove fiber from wait-queue
        i = wait_queue_.erase( i);
        // notify scheduler
        scheduler_->set_ready( f);
    }
}

void
context::join() noexcept {
    // wait for context which is not terminated
    if ( 0 == ( flags_ & flag_terminated) ) {
        // get active context
        context * active_ctx = context::active();
        // push active context to wait-queue, member
        // of the context which has to be joined by
        // the active context
        wait_queue_.push_back( * active_ctx);
        // suspend active context
        scheduler_->re_schedule( active_ctx);
    }
}

bool
context::wait_is_linked() {
    return wait_hook_.is_linked();
}

bool
context::ready_is_linked() {
    return ready_hook_.is_linked();
}

void
context::wait_unlink() {
    wait_hook_.unlink();
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
