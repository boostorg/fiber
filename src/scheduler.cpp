
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/fiber/scheduler.hpp"

#include <cmath>
#include <thread> // std::this_thread::sleep_until()
#include <utility>

#include <boost/assert.hpp>

#include "boost/fiber/algorithm.hpp"
#include "boost/fiber/exceptions.hpp"
#include "boost/fiber/interruption.hpp"
#include "boost/fiber/round_robin.hpp"

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

scheduler::scheduler( context * main_context) noexcept :
    ev_(),
    sched_algo_( new round_robin() ),
    main_context_( main_context),
    ready_queue_(),
    sleep_queue_(),
    terminated_queue_(),
    remote_ready_queue_() {
}

scheduler::~scheduler() noexcept {
    BOOST_ASSERT( context::active() == main_context_);
    for (;;) {
        {
            // move all fibers in remote-ready-queue to runnable-queue
            remote_ready_queue_.consume_all([=](context * f){
                                                f->set_ready();
                                                ready_queue_.push_back( * f);
                                            });
            // move all fibers in ready-queue to running-queue
            ready_queue_t::iterator e = ready_queue_.end();
            for ( ready_queue_t::iterator i = ready_queue_.begin(); i != e;) {
                context * f = & ( * i);
                BOOST_ASSERT( f->is_ready() );

                i = ready_queue_.erase( i);
                BOOST_ASSERT( ! f->runnable_is_linked() );
                BOOST_ASSERT( ! f->ready_is_linked() );
                // timed_mutex::try_lock_until() -> add f to waiting-queue
                //                               -> add f to sleeping-queue
                if ( f->sleep_is_linked() ) {
                    f->sleep_unlink();
                }
                sched_algo_->awakened( f);
            }
        }
        {
            // interrupt all fibers in the sleep-queue
            sleep_queue_t::iterator e = sleep_queue_.end();
            for ( sleep_queue_t::iterator i = sleep_queue_.begin(); i != e;) {
                context * f = & ( * i);
                BOOST_ASSERT( ! f->is_running() );
                BOOST_ASSERT( ! f->is_terminated() );
                // request interruption
                f->request_interruption( true);
                // set to state_ready
                f->set_ready();
                // unlink after accesing iterator
                f->time_point_reset();
                i = sleep_queue_.erase( i);
                // Pass the newly-unlinked context* to sched_algo.
                BOOST_ASSERT( ! f->runnable_is_linked() );
                BOOST_ASSERT( ! f->ready_is_linked() );
                BOOST_ASSERT( ! f->sleep_is_linked() );
                sched_algo_->awakened( f);
            }
        }
        // pop new fiber from runnable-queue
        context * f( sched_algo_->pick_next() );
        if ( nullptr != f) {
            BOOST_ASSERT( ! f->runnable_is_linked() );
            BOOST_ASSERT( ! f->ready_is_linked() );
            BOOST_ASSERT( ! f->sleep_is_linked() );
            BOOST_ASSERT_MSG( f->is_ready(), "fiber with invalid state in ready-queue");
            // set scheduler
            f->set_scheduler( this);
            // add active-fiber to joinig-list of f
            // fiber::join() should not fail because fiber f is in state_ready
            // so main-fiber should be in the waiting-queue of fiber f
            f->join( main_context_);
            // set main-fiber to state_waiting
            main_context_->set_waiting();
            // push main-fiber to waiting-queue
            BOOST_ASSERT( ! main_context_->runnable_is_linked() );
            ready_queue_.push_back( * main_context_);
            // resume fiber f
            resume_( main_context_, f);
        } else if ( sleep_queue_.empty() ) {
            // ready- and waiting-queue are empty so we can quit
            break;
        }
    }
    // destroy terminated fibers from terminated-queue
    for ( context * f : terminated_queue_) {
        BOOST_ASSERT( f->is_terminated() );
        BOOST_ASSERT( ! f->runnable_is_linked() );
        BOOST_ASSERT( ! f->ready_is_linked() );
        BOOST_ASSERT( ! f->sleep_is_linked() );
        BOOST_ASSERT( ! f->wait_is_linked() );
        intrusive_ptr_release( f); // might call ~context()
    }
    terminated_queue_.clear();
    BOOST_ASSERT( context::active() == main_context_);
    BOOST_ASSERT( ready_queue_.empty() );
    BOOST_ASSERT( sleep_queue_.empty() );
    BOOST_ASSERT( terminated_queue_.empty() );
    BOOST_ASSERT( 0 == sched_algo_->ready_fibers() );
}

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
    // push terminated fibers to terminated-queue
    if ( af->is_terminated() ) {
        BOOST_ASSERT( ! af->runnable_is_linked() );
        terminated_queue_.push_back( af);
    }
    // resume active-fiber == f
    f->resume();
}

void
scheduler::spawn( context * f) {
    BOOST_ASSERT( nullptr != f);
    BOOST_ASSERT( f->is_ready() );
    BOOST_ASSERT( f != context::active() );
    BOOST_ASSERT( ! f->runnable_is_linked() );
    BOOST_ASSERT( ! f->ready_is_linked() );
    BOOST_ASSERT( ! f->sleep_is_linked() );
    // add new fiber to the scheduler
    f->set_scheduler( this);
    sched_algo_->awakened( f);
}

void
scheduler::run( context * af) {
    BOOST_ASSERT( nullptr != af);
    BOOST_ASSERT( context::active() == af);
    for (;;) {
        {
            // move all fibers in remote-ready-queue to runnable-queue
            remote_ready_queue_.consume_all([=](context * f){
                                                f->set_ready();
                                                ready_queue_.push_back( * f);
                                            });
            // move all fibers in ready-queue to runnable-queue
            ready_queue_t::iterator e = ready_queue_.end();
            for ( ready_queue_t::iterator i = ready_queue_.begin(); i != e;) {
                context * f = & ( * i);
                BOOST_ASSERT( f->is_ready() );

                i = ready_queue_.erase( i);
                BOOST_ASSERT( ! f->runnable_is_linked() );
                BOOST_ASSERT( ! f->ready_is_linked() );
                // timed_mutex::try_lock_until() -> add f to waiting-queue
                //                               -> add f to sleeping-queue
                if ( f->sleep_is_linked() ) {
                    f->sleep_unlink();
                }
                sched_algo_->awakened( f);
            }
        }
        {
            // move sleeping fibers where the deadline has reached
            // to runnable-queue
            // sleep-queue is sorted (ascending)
            std::chrono::steady_clock::time_point now(
                    std::chrono::steady_clock::now() );
            sleep_queue_t::iterator e( sleep_queue_.end() );
            for ( sleep_queue_t::iterator i( sleep_queue_.begin() ); i != e;) {
                context * f = & ( * i);
                BOOST_ASSERT( ! f->is_running() );
                BOOST_ASSERT( ! f->is_terminated() );
                // set fiber to state_ready if deadline was reached
                if ( f->time_point() <= now) {
                    f->set_ready();
                } else {
                    break; // first element with f->time_point() > now; leave for-loop
                }
                if ( f->is_ready() ) {
                    i = sleep_queue_.erase( i);
                    // unlink only after accessing iterator
                    f->time_point_reset();
                    // Pass the newly-unlinked context* to sched_algo.
                    BOOST_ASSERT( ! f->runnable_is_linked() );
                    BOOST_ASSERT( ! f->ready_is_linked() );
                    BOOST_ASSERT( ! f->sleep_is_linked() );
                    sched_algo_->awakened( f);
                } else {
                    ++i;
                }
            }
        }
        // pop new fiber from runnable-queue
        context * f( sched_algo_->pick_next() );
        if ( nullptr != f) {
            BOOST_ASSERT( ! f->runnable_is_linked() );
            BOOST_ASSERT( ! f->ready_is_linked() );
            BOOST_ASSERT( ! f->sleep_is_linked() );
            BOOST_ASSERT_MSG( f->is_ready(), "fiber with invalid state in runnable-queue");
            // resume fiber f
            resume_( af, f);
            // destroy terminated fibers from terminated-queue
            for ( context * f : terminated_queue_) {
                BOOST_ASSERT( f->is_terminated() );
                BOOST_ASSERT( ! f->runnable_is_linked() );
                BOOST_ASSERT( ! f->ready_is_linked() );
                BOOST_ASSERT( ! f->sleep_is_linked() );
                BOOST_ASSERT( ! f->wait_is_linked() );
                intrusive_ptr_release( f); // might call ~context()
            }
            terminated_queue_.clear();
            return;
        } else {
            // no fibers ready to run
            // set deadline to highest value
            std::chrono::steady_clock::time_point tp(
                    (std::chrono::steady_clock::time_point::max)());
            // get lowest deadline from sleep-queue
            sleep_queue_t::iterator i( sleep_queue_.begin() );
            if ( sleep_queue_.end() != i) {
                tp = i->time_point();
            }
            // wait till signaled or timed-out
            ev_.reset( tp);
        }
    }
}

bool
scheduler::wait_until( context * af,
                       std::chrono::steady_clock::time_point const& timeout_time) {
    BOOST_ASSERT( nullptr != af);
    BOOST_ASSERT( context::active() == af);
    // from this_fiber::sleep() -> running
    // from timed_mutex::lock_until() -> waiting
    BOOST_ASSERT( af->is_running() || af->is_waiting() );
    // set to state_waiting
    af->set_waiting();
    // push active-fiber to sleep-queue
    af->time_point( timeout_time);
    BOOST_ASSERT( ! af->sleep_is_linked() );
    sleep_queue_.insert( * af);
    // switch to another fiber
    run( af);
    // fiber has been resumed
    // check if fiber was interrupted
    this_fiber::interruption_point();
    // check if deadline has reached
    return std::chrono::steady_clock::now() < timeout_time;
}

void
scheduler::yield( context * af) {
    BOOST_ASSERT( nullptr != af);
    BOOST_ASSERT( context::active() == af);
    BOOST_ASSERT( af->is_running() );
    // set active-fiber to state_ready
    af->set_ready();
    // push active-fiber to ready-queue
    BOOST_ASSERT( ! af->ready_is_linked() );
    ready_queue_.push_back( * af);
    // schedule another fiber
    run( af);
    // fiber has been resumed
    // NOTE: do not check if fiber was interrupted
    //       yield() should not be an interruption point
}

void
scheduler::signal( context * f) {
    BOOST_ASSERT( nullptr != f);
    BOOST_ASSERT( ! f->is_terminated() );

    // a fiber MUST NOT be inserted multiple times
    // in runnable- or ready-queue
    if ( ! f->ready_is_linked() &&
         ! f->runnable_is_linked() ) {
        // set fiber to state_ready
        f->set_ready();
        // put reafy fiber ot read-queue
        ready_queue_.push_back( * f);
    }
}

void
scheduler::remote_signal( context * f) {
    BOOST_ASSERT( nullptr != f);
    BOOST_ASSERT( ! f->is_terminated() );

    while ( ! remote_ready_queue_.bounded_push( f) ) {
        context::active()->do_yield();
    }
    ev_.set();
}

std::size_t
scheduler::ready_fibers() const noexcept {
    return sched_algo_->ready_fibers();
}

void
scheduler::set_sched_algo( std::unique_ptr< sched_algorithm > algo) {
    sched_algo_ = std::move( algo);
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
