
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
    splk_(),
    waiting_() {
}

condition::~condition() {
    BOOST_ASSERT( waiting_.empty() );
}

void
condition::notify_one() {
    context * f( nullptr);

    detail::spinlock_lock lk( splk_);
    // get one waiting fiber
    if ( ! waiting_.empty() ) {
        f = & waiting_.front();
        waiting_.pop_front();
    }
    lk.unlock();

    // notify waiting fiber
    if ( nullptr != f) {
        f->set_ready();
    }
}

void
condition::notify_all() {
    wqueue_t waiting;

    detail::spinlock_lock lk( splk_);
    // get all waiting fibers
    waiting.swap( waiting_);
    lk.unlock();

    // notify all waiting fibers
    for ( context & f : waiting) {
        f.set_ready();
        // f->wait_unlink(); ?
    }
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
