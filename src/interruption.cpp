
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
//  based on boost.thread

#include "boost/fiber/interruption.hpp"

#include "boost/fiber/context.hpp"
#include "boost/fiber/scheduler.hpp"
#include "boost/fiber/exceptions.hpp"

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace this_fiber {

disable_interruption::disable_interruption() noexcept :
    set_{ fibers::context::active()->interruption_blocked() } {
    if ( ! set_) {
        fibers::context::active()->interruption_blocked( true);
    }
}

disable_interruption::~disable_interruption() {
    if ( ! set_) {
        fibers::context::active()->interruption_blocked( false);
    }
}

restore_interruption::restore_interruption( disable_interruption & disabler) noexcept :
    disabler_{ disabler } {
    if ( ! disabler_.set_) {
        fibers::context::active()->interruption_blocked( false);
    }
}

restore_interruption::~restore_interruption() {
    if ( ! disabler_.set_) {
        fibers::context::active()->interruption_blocked( true);
    }
}

BOOST_FIBERS_DECL
bool interruption_enabled() noexcept { 
    return ! fibers::context::active()->interruption_blocked(); 
} 
 
BOOST_FIBERS_DECL
bool interruption_requested() noexcept { 
    return fibers::context::active()->interruption_requested(); 
}

BOOST_FIBERS_DECL
void interruption_point() {
    if ( interruption_requested() && interruption_enabled() ) {
        fibers::context::active()->request_interruption( false);
        throw fibers::fiber_interrupted();
    }
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
