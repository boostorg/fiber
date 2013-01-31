
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_FIBERS_SOURCE

#include <boost/fiber/mutex.hpp>

#include <algorithm>

#include <boost/assert.hpp>

#include <boost/fiber/detail/scheduler.hpp>
#include <boost/fiber/interruption.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

mutex::mutex() :
	state_( UNLOCKED),
    waiting_mtx_(),
    waiting_()
{}

mutex::~mutex()
{ BOOST_ASSERT( waiting_.empty() ); }

void
mutex::lock()
{
    detail::fiber_base::ptr_t f( detail::scheduler::instance().active() );
    if ( LOCKED == state_.exchange( LOCKED, memory_order_seq_cst) )
    {
        // store this fiber in order to be notified later
        unique_lock< detail::spinlock > lk( waiting_mtx_);
        waiting_.push_back( f);
        lk.unlock();
        do
        {
            try
            {
                if ( f)
                {
                    // suspend this fiber
                    detail::scheduler::instance().wait();
                }
                else
                {
                    // run scheduler
                    detail::scheduler::instance().run();
                }
            }
            catch (...)
            {
                // FIXME: use multi-index container
                // remove fiber from waiting_
                unique_lock< detail::spinlock > lk( waiting_mtx_);
                waiting_.erase(
                    std::find( waiting_.begin(), waiting_.end(), f) );
                throw;
            }
        }
        while ( LOCKED == state_.exchange( LOCKED, memory_order_seq_cst) );
    }
}

bool
mutex::try_lock()
{ return UNLOCKED == state_.exchange( LOCKED, memory_order_seq_cst); }

void
mutex::unlock()
{
	state_ = UNLOCKED;

    unique_lock< detail::spinlock > lk( waiting_mtx_);
	if ( ! waiting_.empty() )
    {
        waiting_.front()->wake_up();
        waiting_.pop_front();
    }
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
