
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_THIS_STRATUM_INTERRUPTION_H
#define BOOST_THIS_STRATUM_INTERRUPTION_H

#include <cstddef>

#include <boost/config.hpp>
#include <boost/utility.hpp>

#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/interrupt_flags.hpp>
#include <boost/fiber/scheduler.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace this_fiber {

class restore_interruption;

class disable_interruption : private noncopyable
{
private:
	friend class restore_interruption;

	bool	set_;

public:
	disable_interruption() :
		set_( ( fibers::scheduler::get_instance_()->active_fibers_->impl_->interrupt & fibers::detail::INTERRUPTION_BLOCKED) != 0)
	{
		if ( ! set_)
			fibers::scheduler::get_instance_()->active_fibers_->impl_->interrupt |= fibers::detail::INTERRUPTION_BLOCKED;
	}

	~disable_interruption()
	{
		try
		{
			if ( ! set_)
				fibers::scheduler::get_instance_()->active_fibers_->impl_->interrupt &= ~fibers::detail::INTERRUPTION_BLOCKED;
		}
		catch (...)
		{}
	}
};

class restore_interruption : private noncopyable
{
private:
	disable_interruption	&	disabler_;

public:
	explicit restore_interruption( disable_interruption & disabler) :
		disabler_( disabler)
	{
		if ( ! disabler_.set_)
			fibers::scheduler::get_instance_()->active_fibers_->impl_->interrupt &= ~fibers::detail::INTERRUPTION_BLOCKED;
	}

	~restore_interruption()
	{
	   try
	   {	   
			if ( ! disabler_.set_)
				fibers::scheduler::get_instance_()->active_fibers_->impl_->interrupt |= fibers::detail::INTERRUPTION_BLOCKED;
		}
		catch (...)
		{}
	}
};

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_THIS_STRATUM_INTERRUPTION_H
