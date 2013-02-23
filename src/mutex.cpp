
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

const int mutex::LOCKED = 0;
const int mutex::UNLOCKED = 1;

mutex::mutex() :
	state_( UNLOCKED),
    splk_(),
    waiting_()
{}

mutex::~mutex()
{ BOOST_ASSERT( waiting_.empty() ); }

void
mutex::lock()
{
    while ( LOCKED == state_.exchange( LOCKED) )
    {
        detail::notify::ptr_t n( detail::scheduler::instance().active() );
        try
        {
            if ( n)
            {
                // store this fiber in order to be notified later
                unique_lock< detail::spinlock > lk( splk_);
                waiting_.push_back( n);

                // suspend this fiber
                detail::scheduler::instance().wait( lk);
            }
            else
            {
                // notifier for main-fiber
                n = detail::scheduler::instance().notifier();
                // store this fiber in order to be notified later
                unique_lock< detail::spinlock > lk( splk_);
                waiting_.push_back( n);

                lk.unlock();
                while ( ! n->is_ready() )
                {
                    // run scheduler
                    detail::scheduler::instance().run();
                }
            }
        }
        catch (...)
        {
            // remove fiber from waiting_
            unique_lock< detail::spinlock > lk( splk_);
            waiting_.erase(
                std::find( waiting_.begin(), waiting_.end(), n) );
            throw;
        }
    }
}

bool
mutex::try_lock()
{ return UNLOCKED == state_.exchange( LOCKED); }

void
mutex::unlock()
{
    detail::notify::ptr_t n;

    unique_lock< detail::spinlock > lk( splk_);
    if ( ! waiting_.empty() ) {
        n.swap( waiting_.front() );
        waiting_.pop_front();
    }
    lk.unlock();

    if ( n)
        n->set_ready();

	state_ = UNLOCKED;
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
