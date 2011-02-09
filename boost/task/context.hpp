
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TASKS_CONTEXT_H
#define BOOST_TASKS_CONTEXT_H

#include <boost/atomic.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread_time.hpp>
#include <boost/utility.hpp>

#include <boost/task/detail/config.hpp>

#include <boost/config/abi_prefix.hpp>

# if defined(BOOST_MSVC)
# pragma warning(push)
# pragma warning(disable:4251 4275)
# endif

namespace boost {
namespace tasks {
namespace detail {

class BOOST_TASKS_DECL context_base : private noncopyable
{
private:
	atomic< unsigned int >	use_count_;
	bool					requested_;
	mutex					mtx_;
	shared_ptr< thread >	thrd_;

	void reset_( shared_ptr< thread > const& thrd);

	void interrupt_();

public:
	context_base();

	void reset( shared_ptr< thread > const& thrd);

	void interrupt();

	bool interruption_requested();

	inline friend void intrusive_ptr_add_ref( context_base * p)
	{ p->use_count_.fetch_add( 1, memory_order_relaxed); }
	
	inline friend void intrusive_ptr_release( context_base * p)
	{
		if ( p->use_count_.fetch_sub( 1, memory_order_release) == 1)
		{
			atomic_thread_fence( memory_order_acquire);
			delete p;
		}
	}
};

}

class BOOST_TASKS_DECL context
{
private:
	intrusive_ptr< detail::context_base >	base_;

public:
	context();

	void reset( shared_ptr< thread > const& thrd);

	void interrupt();

	bool interruption_requested();

	void swap( context & other);
};

}}

# if defined(BOOST_MSVC)
# pragma warning(pop)
# endif

#include <boost/config/abi_suffix.hpp>

#endif // BOOST_TASKS_DETAIL_context_H
