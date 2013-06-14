
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_FIBERS_SOURCE

#include <boost/fiber/mutex.hpp>

#include <algorithm>

#include <boost/assert.hpp>

#include <boost/fiber/detail/main_notifier.hpp>
#include <boost/fiber/detail/scheduler.hpp>
#include <boost/fiber/interruption.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

mutex::mutex() :
	state_( UNLOCKED),
    waiting_()
{}

mutex::~mutex()
{ BOOST_ASSERT( waiting_.empty() ); }

void
mutex::lock()
{
    while ( LOCKED == state_)
    {
        detail::notify::ptr_t n( detail::scheduler::instance().active() );
        try
        {
            if ( n)
            {
                // store this fiber in order to be notified later
                waiting_.push_back( n);

                // suspend this fiber
                detail::scheduler::instance().wait();
            }
            else
            {
                // notifier for main-fiber
                detail::main_notifier mn;
                n = detail::main_notifier::make_pointer( mn);

                // store this fiber in order to be notified later
                waiting_.push_back( n);

                // wait until main-fiber gets notified
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
            waiting_.erase(
                std::find( waiting_.begin(), waiting_.end(), n) );
            throw;
        }
    }
    state_ = LOCKED;
}

bool
mutex::try_lock()
{
    if ( UNLOCKED == state_)
    {
        state_ = LOCKED;
        return true;
    }
    return false;
}

void
mutex::unlock()
{
    detail::notify::ptr_t n;

    if ( ! waiting_.empty() ) {
        n.swap( waiting_.front() );
        waiting_.pop_front();
    }

	state_ = UNLOCKED;

    if ( n)
        n->set_ready();
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
