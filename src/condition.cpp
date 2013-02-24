
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

    unique_lock< detail::spinlock > lk( splk_);
    if ( ! waiting_.empty() ) {
        n.swap( waiting_.front() );
        waiting_.pop_front();
    }

    if ( n)
        n->set_ready();
}

void
condition::notify_all()
{
    detail::notify::ptr_t n;

    unique_lock< detail::spinlock > lk( splk_);
    while ( ! waiting_.empty() )
    {
        n.swap( waiting_.front() );
        waiting_.pop_front();
        n->set_ready();
    }
#if 0
    BOOST_FOREACH( detail::notify::ptr_t const& n, waiting_)
    {
        n->set_ready();
    }
    waiting_.clear();
#endif
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
