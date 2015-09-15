
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
scheduler::resume_( context * af, context * f) {
    BOOST_ASSERT( nullptr != af);
    BOOST_ASSERT( nullptr != f);
    BOOST_ASSERT( f->is_ready() );
    // set fiber to state running
    f->set_running();
    // fiber next-to-run is same as current active-fiber
    // this might happen in context of this_fiber::yield() 
    if ( f == af) {
        return;
    }
    // pass new fiber the scheduler of the current active fiber
    // this might be necessary if the new fiber was migrated
    // from another thread
    // FIXME: mabye better don in the sched-algorithm (knows
    //        if fiber was migrated)
    //        -> performance issue?
    f->set_scheduler( af->get_scheduler() );
    // assign new fiber to active-fiber
    context::active( f);
    // resume active-fiber == f
    f->resume();
}

scheduler::scheduler() noexcept :
    main_ctx_( nullptr),
    dispatching_ctx_( nullptr),
    ready_queue_() {
}

scheduler::~scheduler() noexcept {
    BOOST_ASSERT( nullptr != main_ctx_);
    BOOST_ASSERT( dispatching_ctx_);
    BOOST_ASSERT( context::active() == main_ctx_);

    // FIXME: signal dispatching context termination
    //        wait till dispatching context agrees on termination

    // reset intrusive-pointer to dispatching context
    // should destroy dispatching context stack
    dispatching_ctx_.reset();
}

void
scheduler::main_context( context * main_ctx) noexcept {
    BOOST_ASSERT( nullptr != main_ctx);
    main_ctx_ = main_ctx;
}

void
scheduler::dispatching_context( intrusive_ptr< context > dispatching_ctx) noexcept {
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
scheduler::dispatch() noexcept {
    if ( ready_queue.empty() ) {
        // FIXME: sleep
    }
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
