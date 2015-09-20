
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/fiber/condition.hpp"

#include "boost/fiber/context.hpp"

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

condition::condition() :
    wait_queue_(),
    wait_queue_splk_() {
}

condition::~condition() {
    BOOST_ASSERT( wait_queue_.empty() );
}

void
condition::notify_one() {
    context * ctx( nullptr);
    // get one context' from wait-queue
    detail::spinlock_lock lk( wait_queue_splk_);
    if ( ! wait_queue_.empty() ) {
        ctx = & wait_queue_.front();
        wait_queue_.pop_front();
    }
    lk.unlock();
    // notify context
    if ( nullptr != ctx) {
        context::active()->set_ready( ctx);
    }
}

void
condition::notify_all() {
    wait_queue_t tmp;
    // get all context' from wait-queue
    detail::spinlock_lock lk( wait_queue_splk_);
    tmp.swap( wait_queue_);
    lk.unlock();
    // notify all context'
    while ( ! tmp.empty() ) {
        context * ctx = & tmp.front();
        tmp.pop_front();
        BOOST_ASSERT( nullptr != ctx);
        context::active()->set_ready( ctx);
    }
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
