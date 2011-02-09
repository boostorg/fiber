
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TASKS_CALLABLE_H
#define BOOST_TASKS_CALLABLE_H

#include <boost/config.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/move/move.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

#include <boost/task/context.hpp>
#include <boost/task/detail/config.hpp>

#include <boost/config/abi_prefix.hpp>

# if defined(BOOST_MSVC)
# pragma warning(push)
# pragma warning(disable:4251 4275)
# endif

namespace boost {
namespace tasks {
namespace detail {

struct BOOST_TASKS_DECL callable_base
{
	atomic< unsigned int >	use_count;

	callable_base() :
		use_count( 0)
	{}

	virtual ~callable_base() {}

	virtual void run() = 0;

	virtual void reset( shared_ptr< thread > const&) = 0;

	inline friend void intrusive_ptr_add_ref( callable_base * p)
	{ p->use_count.fetch_add( 1, memory_order_relaxed); }
	
	inline friend void intrusive_ptr_release( callable_base * p)
	{
		if ( p->use_count.fetch_sub( 1, memory_order_release) == 1)
		{
			atomic_thread_fence( memory_order_acquire);
			delete p;
		}
	}
};

template< typename Task, typename Promise >
class callable_object : public callable_base
{
private:
	Task		t_;
	context		ctx_;

public:
#ifdef BOOST_HAS_RVALUE_REFS
	callable_object(
			Task && t,
			Promise && prom,
			context const& ctx) :
		t_( t), ctx_( ctx)
	{ t_.set_promise( prom); }
#else
	callable_object(
			BOOST_RV_REF( Task) t,
			boost::detail::thread_move_t< Promise > prom,
			context const& ctx) :
		t_( t), ctx_( ctx)
	{ t_.set_promise( prom); }
#endif

	callable_object(
			BOOST_RV_REF( Task) t,
			BOOST_RV_REF( Promise) prom,
			context const& ctx) :
		t_( t), ctx_( ctx)
	{ t_.set_promise( prom); }

	void run()
	{ t_(); }

	void reset( shared_ptr< thread > const& thrd)
	{ ctx_.reset( thrd); }
};

}

class BOOST_TASKS_DECL callable
{
private:
	intrusive_ptr< detail::callable_base >	base_;

public:
	callable();

#ifdef BOOST_HAS_RVALUE_REFS
	template< typename Task, typename Promise >
	callable(
			Task && t,
			Promise && prom,
			context const& ctx) :
		base_( new detail::callable_object< Task, Promise >( t, prom, ctx) )
	{}
#else
	template< typename Task, typename Promise >
	callable(
			BOOST_RV_REF( Task) t,
			boost::detail::thread_move_t< Promise > prom,
			context const& ctx) :
		base_( new detail::callable_object< Task, Promise >( t, prom, ctx) )
	{}
#endif

	template< typename Task, typename Promise >
	callable(
			BOOST_RV_REF( Task) t,
			BOOST_RV_REF( Promise) prom,
			context const& ctx) :
		base_( new detail::callable_object< Task, Promise >( t, prom, ctx) )
	{}

	void operator()();

	bool empty() const;

	void clear();

	void reset( shared_ptr< thread > const&);

	void swap( callable &);
};

class context_guard : private noncopyable
{
private:
	callable	&	ca_;

public:
	context_guard( callable & ca, shared_ptr< thread > const& thrd) :
		ca_( ca)
	{ ca_.reset( thrd); }

	~context_guard()
	{ ca_.clear(); }
};

}}

# if defined(BOOST_MSVC)
# pragma warning(pop)
# endif

#include <boost/config/abi_suffix.hpp>

#endif // BOOST_TASKS_CALLABLE_H

