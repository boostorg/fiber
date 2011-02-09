
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TASKLETS_DETAIL_TASKLET_BASE_H
#define BOOST_TASKLETS_DETAIL_TASKLET_BASE_H

#include <cstddef>

#include <boost/config.hpp>
#include <boost/fiber.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/utility.hpp>

#include <boost/tasklet/detail/config.hpp>
#include <boost/tasklet/detail/interrupt_flags.hpp>
#include <boost/tasklet/detail/state_flags.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace tasklets {

class strategy;

namespace detail {

BOOST_TASKLET_DECL void trampoline( void *);

class BOOST_TASKLET_DECL tasklet_base : private noncopyable
{
public:
	typedef intrusive_ptr< tasklet_base >	ptr;

	friend BOOST_TASKLET_DECL void trampoline( void *);

	template< typename AllocatorT >
	tasklet_base( std::size_t stacksize, AllocatorT const& alloc) :
		use_count_( 0),
		fib_( trampoline, this, stacksize, alloc),
		priority_( 0),
		state_( STATE_NOT_STARTED),
		interrupt_( INTERRUPTION_DISABLED),
		st_( 0)
	{}

	virtual ~tasklet_base() {}

	void run();

	void yield();

	int priority() const;

	void priority( int);

	state_type state() const;

	void state( state_type);

	interrupt_type & interrupt();

	void interrupt( interrupt_type);

	strategy * attached_strategy();

	void attached_strategy( strategy *);

	friend inline void intrusive_ptr_add_ref( tasklet_base * p)
	{ ++( p->use_count_); }

	friend inline void intrusive_ptr_release( tasklet_base * p)
	{ if ( 0 == --( p->use_count_) ) delete p; }

protected:
	virtual void exec() = 0;

private:
	std::size_t			use_count_;
	fiber				fib_;
	int					priority_;
	state_type			state_;
	interrupt_type		interrupt_;
	strategy		*	st_;
};

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_TASKLETS_DETAIL_TASKLET_BASE_H
