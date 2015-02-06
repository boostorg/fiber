
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/fiber/condition.hpp"

#include "boost/fiber/fiber_context.hpp"

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
    fiber_context * f( nullptr);

    std::unique_lock< detail::spinlock > lk( splk_);
    // get one waiting fiber
    if ( ! waiting_.empty() ) {
        f = waiting_.front();
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
    std::deque< fiber_context * > waiting;

    std::unique_lock< detail::spinlock > lk( splk_);
    // get all waiting fibers
    waiting.swap( waiting_);
    lk.unlock();

    // notify all waiting fibers
    while ( ! waiting.empty() ) {
        fiber_context * f( waiting.front() );
        waiting.pop_front();
        BOOST_ASSERT( nullptr != f);
        f->set_ready();
    }
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
