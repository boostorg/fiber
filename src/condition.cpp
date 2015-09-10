
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
    wait_queue_() {
}

condition::~condition() {
    BOOST_ASSERT( wait_queue_.empty() );
}

void
condition::notify_one() {
    context * f( nullptr);

    detail::spinlock_lock lk( splk_);
    // get one waiting fiber
    if ( ! wait_queue_.empty() ) {
        f = & wait_queue_.front();
        wait_queue_.pop_front();
    }
    lk.unlock();

    // notify waiting fiber
    if ( nullptr != f) {
        context::active()->do_signal( f);
    }
}

void
condition::notify_all() {
    wait_queue_t tmp;

    detail::spinlock_lock lk( splk_);
    // get all waiting fibers
    tmp.swap( wait_queue_);
    lk.unlock();

    // notify all waiting fibers
    for ( context & f : tmp) {
        context::active()->do_signal( & f);
    }
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
