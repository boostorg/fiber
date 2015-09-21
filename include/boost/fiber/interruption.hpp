
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// based on boost.thread

#ifndef BOOST_THIS_FIBER_INTERRUPTION_H
#define BOOST_THIS_FIBER_INTERRUPTION_H

#include <boost/config.hpp>

#include <boost/fiber/detail/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace this_fiber {

class restore_interruption;

class BOOST_FIBERS_DECL disable_interruption {
private:
	friend class restore_interruption;

	bool	set_;

public:
	disable_interruption() noexcept;

	~disable_interruption() noexcept;

    disable_interruption( disable_interruption const&) = delete;
    disable_interruption & operator=( disable_interruption const&) = delete;
};

class BOOST_FIBERS_DECL restore_interruption {
private:
	disable_interruption	&	disabler_;

public:
	explicit restore_interruption( disable_interruption & disabler) noexcept;

	~restore_interruption() noexcept;

    restore_interruption( restore_interruption const&) = delete;
    restore_interruption & operator=( restore_interruption const&) = delete;
};

BOOST_FIBERS_DECL
bool interruption_enabled() noexcept;

BOOST_FIBERS_DECL
bool interruption_requested() noexcept;

BOOST_FIBERS_DECL
void interruption_point();

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_THIS_FIBER_INTERRUPTION_H
