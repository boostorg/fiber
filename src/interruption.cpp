
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
//  based on boost.thread

#define BOOST_FIBERS_SOURCE

#include <boost/fiber/interruption.hpp>

#include <boost/throw_exception.hpp>

#include <boost/fiber/detail/interrupt_flags.hpp>
#include <boost/fiber/detail/scheduler.hpp>
#include <boost/fiber/exceptions.hpp>
#include <boost/fiber/operations.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace this_fiber {

disable_interruption::disable_interruption() BOOST_NOEXCEPT :
    set_( ( fibers::detail::scheduler::instance()->active()->interruption_blocked() ) )
{
    if ( ! set_)
        fibers::detail::scheduler::instance()->active()->interruption_blocked( true);
}

disable_interruption::~disable_interruption() BOOST_NOEXCEPT
{
    if ( ! set_)
        fibers::detail::scheduler::instance()->active()->interruption_blocked( false);
}

restore_interruption::restore_interruption( disable_interruption & disabler) BOOST_NOEXCEPT :
    disabler_( disabler)
{
    if ( ! disabler_.set_)
        fibers::detail::scheduler::instance()->active()->interruption_blocked( false);
}

restore_interruption::~restore_interruption() BOOST_NOEXCEPT
{
    if ( ! disabler_.set_)
        fibers::detail::scheduler::instance()->active()->interruption_blocked( true);
}

bool interruption_enabled() BOOST_NOEXCEPT 
{ 
    fibers::detail::fiber_base::ptr_t f( fibers::detail::scheduler::instance()->active() );
    return f && f->interruption_enabled(); 
} 
 
bool interruption_requested() BOOST_NOEXCEPT 
{ 
    fibers::detail::fiber_base::ptr_t f( fibers::detail::scheduler::instance()->active() );
    if ( ! f) return false; 
    return f->interruption_requested(); 
}

void interruption_point()
{
    if ( interruption_requested() && interruption_enabled() )
    {
        fibers::detail::scheduler::instance()->active()->request_interruption( false);
        boost::throw_exception( fibers::fiber_interrupted() );
    }
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
