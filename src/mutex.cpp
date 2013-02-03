
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

#include <cstdio>

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
    if ( LOCKED == state_.exchange( LOCKED, memory_order_seq_cst) )
    {
        detail::notify::ptr_t n( detail::scheduler::instance().active() );
        try
        {
            if ( n)
            {
                // store this fiber in order to be notified later
                unique_lock< detail::spinlock > lk( waiting_mtx_);
                waiting_.push_back( n);

                // suspend this fiber
                detail::scheduler::instance().wait( lk);
            }
            else
            {
                // notifier for main-fiber
                n = detail::scheduler::instance().notifier();
                // store this fiber in order to be notified later
                unique_lock< detail::spinlock > lk( waiting_mtx_);
                waiting_.push_back( n);

                lk.unlock();
                while ( ! n->woken_up() )
                {
                    fprintf(stdout, "mutex: main-fiber not woken-up\n");
                    // run scheduler
                    detail::scheduler::instance().run();
                }
                    fprintf(stdout, "mutex: main-fiber woken-up\n");
            }
        }
        catch (...)
        {
            // remove fiber from waiting_
            unique_lock< detail::spinlock > lk( waiting_mtx_);
            waiting_.erase(
                std::find( waiting_.begin(), waiting_.end(), n) );
            throw;
        }
    }
}

bool
mutex::try_lock()
{ return UNLOCKED == state_.exchange( LOCKED, memory_order_seq_cst); }

void
mutex::unlock()
{
    detail::notify::ptr_t n;

    unique_lock< detail::spinlock > lk( waiting_mtx_);
    if ( ! waiting_.empty() ) {
        n.swap( waiting_.front() );
        waiting_.pop_front();
    }
    lk.unlock();

	state_ = UNLOCKED;

    if ( n)
        n->wake_up();
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
