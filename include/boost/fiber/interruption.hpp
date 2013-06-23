
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// based on boost.thread

#ifndef BOOST_THIS_FIBER_INTERRUPTION_H
#define BOOST_THIS_FIBER_INTERRUPTION_H

#include <cstddef>

#include <boost/config.hpp>
#include <boost/utility.hpp>

#include <boost/fiber/detail/config.hpp>

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
	disable_interruption() BOOST_NOEXCEPT;

	~disable_interruption() BOOST_NOEXCEPT;
};

class restore_interruption : private noncopyable
{
private:
	disable_interruption	&	disabler_;

public:
	explicit restore_interruption( disable_interruption & disabler) BOOST_NOEXCEPT;

	~restore_interruption() BOOST_NOEXCEPT;
};

bool interruption_enabled() BOOST_NOEXCEPT;

bool interruption_requested() BOOST_NOEXCEPT;

void interruption_point();

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_THIS_FIBER_INTERRUPTION_H
