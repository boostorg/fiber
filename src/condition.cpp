
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_FIBERS_SOURCE

#include "boost/fiber/condition.hpp"

#include <boost/foreach.hpp>

#include <boost/fiber/detail/scheduler.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

condition::condition() :
    splk_(),
    waiting_()
{}

condition::~condition()
{ BOOST_ASSERT( waiting_.empty() ); }

void
condition::notify_one()
{
    detail::notify::ptr_t n;

    // get one waiting fiber
    unique_lock< detail::spinlock > lk( splk_);
    if ( ! waiting_.empty() ) {
        n.swap( waiting_.front() );
        waiting_.pop_front();
    }
    lk.unlock();

    // notify waiting fiber
    if ( n)
        n->set_ready();
}

void
condition::notify_all()
{
    std::deque< detail::notify::ptr_t > waiting;

    // get all waiting fibers
    unique_lock< detail::spinlock > lk( splk_);
    waiting.swap( waiting_);
    lk.unlock();

    // notify all waiting fibers
    while ( ! waiting.empty() )
    {
        detail::notify::ptr_t n( waiting.front() );
        waiting.pop_front();
        BOOST_ASSERT( n);
        n->set_ready();
    }
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
