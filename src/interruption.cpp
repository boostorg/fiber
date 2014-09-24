
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
//  based on boost.thread

#include "boost/fiber/interruption.hpp"

#include <boost/throw_exception.hpp>

#include "boost/fiber/detail/fiber_base.hpp"
#include "boost/fiber/detail/interrupt_flags.hpp"
#include "boost/fiber/fiber_manager.hpp"
#include "boost/fiber/exceptions.hpp"
#include "boost/fiber/operations.hpp"

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace this_fiber {

disable_interruption::disable_interruption() BOOST_NOEXCEPT :
    set_( ( fibers::fm_active()->interruption_blocked() ) )
{
    if ( ! set_)
        fibers::fm_active()->interruption_blocked( true);
}

disable_interruption::~disable_interruption() BOOST_NOEXCEPT
{
    if ( ! set_)
        fibers::fm_active()->interruption_blocked( false);
}

restore_interruption::restore_interruption( disable_interruption & disabler) BOOST_NOEXCEPT :
    disabler_( disabler)
{
    if ( ! disabler_.set_)
        fibers::fm_active()->interruption_blocked( false);
}

restore_interruption::~restore_interruption() BOOST_NOEXCEPT
{
    if ( ! disabler_.set_)
        fibers::fm_active()->interruption_blocked( true);
}

bool interruption_enabled() BOOST_NOEXCEPT 
{ 
    fibers::detail::fiber_base * f = fibers::fm_active();
    return 0 != f && ! f->interruption_blocked(); 
} 
 
bool interruption_requested() BOOST_NOEXCEPT 
{ 
    fibers::detail::fiber_base * f = fibers::fm_active();
    if ( 0 == f) return false; 
    return f->interruption_requested(); 
}

void interruption_point()
{
    if ( interruption_requested() && interruption_enabled() )
    {
        fibers::fm_active()->request_interruption( false);
        boost::throw_exception( fibers::fiber_interrupted() );
    }
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
