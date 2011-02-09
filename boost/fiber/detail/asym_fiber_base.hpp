
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_DETAIL_ASYM_FIBER_BASE_H
#define BOOST_FIBERS_DETAIL_ASYM_FIBER_BASE_H

#include <cstddef>
#include <stack>

#include <boost/config.hpp>
#include <boost/context/all.hpp>
#include <boost/function.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/utility.hpp>

#include <boost/fiber/detail/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

#if defined(BOOST_MSVC)
#pragma warning(push)
#pragma warning(disable:4251)
#pragma warning(disable:4275)
#endif

namespace boost {
namespace fibers {

namespace detail {

BOOST_FIBER_DECL void trampoline_asym( void *);

class BOOST_FIBER_DECL asym_fiber_base : private noncopyable
{
public:
	typedef intrusive_ptr< asym_fiber_base >		ptr;

	struct do_not_return_t {};

	struct do_return_t {};

	static do_not_return_t	do_not_return;
	static do_return_t		do_return;

	friend BOOST_FIBER_DECL void trampoline_asym( void * vp);

	friend inline void intrusive_ptr_add_ref( asym_fiber_base * p)
	{ ++p->use_count_; }

	friend inline void intrusive_ptr_release( asym_fiber_base * p)
	{ if ( --p->use_count_ == 0) delete p; }

	asym_fiber_base( std::size_t stacksize, do_not_return_t);

	asym_fiber_base( std::size_t stacksize, do_return_t);

	virtual ~asym_fiber_base() {}

	void run();

	void yield();

	void set_finished();

	bool get_finished() const;

protected:
	virtual void exec() = 0;

private:
	unsigned int		use_count_;
	bool				finished_;
	context<>			caller_;
	context<>			callee_;
};

}}}

#if defined(BOOST_MSVC)
#pragma warning(pop)
#endif

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DETAIL_ASYM_FIBER_BASE_H
