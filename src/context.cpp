
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/fiber/context.hpp"

#include "boost/fiber/fixedsize_stack.hpp"
#include "boost/fiber/scheduler.hpp"

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

static context * main_context() {
    // main fiber context for this thread
    static thread_local context main_ctx;
    // scheduler for this thread
    static thread_local scheduler sched;
    // attach scheduler to main fiber context
    main_ctx.set_scheduler( & sched);
    // attach main fiber context to scheduler
    sched.main_context( & main_ctx);
    // attach dispatching fiber context to scheduler
    sched.dispatching_context(
        make_context(
            fixedsize_stack(),
            & scheduler::dispatch, & sched) );
    return & main_ctx;
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
    BOOST_ASSERT( is_running() ); // set by the scheduler-algorithm
    ctx_();
}

void
context::release() noexcept {
    BOOST_ASSERT( is_terminated() );

    // notify all waiting fibers
    wait_queue_t::iterator e = wait_queue_.end();
    for ( wait_queue_t::iterator i = wait_queue_.begin(); i != e;) {
        context * f = & ( * i);
        i = wait_queue_.erase( i);
        //FIXME: signal that f might resume
    }
}

bool
context::wait_is_linked() {
    return wait_hook_.is_linked();
}

void
context::wait_unlink() {
    wait_hook_.unlink();
}

bool
context::ready_is_linked() {
    return ready_hook_.is_linked();
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
