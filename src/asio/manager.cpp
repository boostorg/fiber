
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/fiber/asio/manager.hpp>

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>

#include <boost/assert.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/scope_exit.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/thread.hpp>

#include <boost/fiber/detail/scheduler.hpp>
#include <boost/fiber/exceptions.hpp>

#include <boost/fiber/round_robin.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace asio {

bool fetch_ready( detail::worker_fiber * f)
{
    BOOST_ASSERT( ! f->is_running() );
    BOOST_ASSERT( ! f->is_terminated() );

    // set fiber to state_ready if dead-line was reached
    // set fiber to state_ready if interruption was requested
    if ( f->time_point() <= clock_type::now() || f->interruption_requested() )
        f->set_ready();
    return f->is_ready();
}

void
manager::evaluate_( detail::worker_fiber * f) {
    BOOST_ASSERT( 0 != f);

    if ( f->is_waiting() )
        wqueue_.push( f);
#if 0
        wqueue_.push(
            schedulable(
                f,
                boost::asio::io_service::work( io_svc_) ) );
#endif
    else if ( f->is_ready() ) resume_( f);
    else BOOST_ASSERT_MSG( false, "fiber with invalid state in ready-queue");
}

manager::manager( boost::asio::io_service & io_svc) BOOST_NOEXCEPT :
    io_svc_( io_svc)
{}

void
manager::run()
{
    for (;;)
    {
        // move all fibers witch are ready (state_ready)
        // from waiting-queue to the runnable-queue
        wqueue_.move_to( sched_algo_, fetch_ready);

        // pop new fiber from ready-queue which is not complete
        // (example: fiber in ready-queue could be canceled by active-fiber)
        detail::worker_fiber * f( sched_algo_->pick_next() );
        if ( f)
        {
            BOOST_ASSERT_MSG( f->is_ready(), "fiber with invalid state in ready-queue");
            io_svc_.post(
                boost::bind( & manager::evaluate_, this, f) );
        }

        if ( 0 == io_svc_.poll_one() ) return;
    }
}

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
