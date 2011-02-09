
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
//  parts are based on boost.future by Anthony Williams

#ifndef BOOST_TASKS_HANDLE_H
#define BOOST_TASKS_HANDLE_H

#include <boost/atomic.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/thread/thread_time.hpp>
#include <boost/utility/enable_if.hpp>

#include <boost/task/context.hpp>
#include <boost/task/spin/future.hpp>
#include <boost/task/exceptions.hpp>

#include <boost/config/abi_prefix.hpp>

namespace boost {
namespace tasks {
namespace detail {

template< typename R >
struct handle_base
{
	template< typename X >
	friend void intrusive_ptr_add_ref( handle_base< X > *);
	template< typename X >
	friend void intrusive_ptr_release( handle_base< X > *);

	typedef intrusive_ptr< handle_base >	ptr;

	atomic< unsigned int >	use_count;

	virtual ~handle_base() {}

	virtual void interrupt() = 0;

	virtual void interrupt_and_wait() = 0;

	virtual bool interrupt_and_wait_until( system_time const& abs_time) = 0;

	virtual bool interruption_requested() = 0;

	virtual R get() = 0;

	virtual bool is_ready() const = 0;

	virtual bool has_value() const = 0;

	virtual bool has_exception() const = 0;

	virtual void wait() const = 0;

	virtual bool wait_until( system_time const& abs_time) const = 0;
};

template< typename R >
void intrusive_ptr_add_ref( handle_base< R > * p)
{ p->use_count.fetch_add( 1, memory_order_relaxed); }

template< typename R >
void intrusive_ptr_release( handle_base< R > * p)
{
	if ( p->use_count.fetch_sub( 1, memory_order_release) == 1)
	{
		atomic_thread_fence( memory_order_acquire);
		delete p;
	}
}

template< typename R, typename F >
class handle_object : public handle_base< R >
{
private:
	F			fut_;
	context		ctx_;

public:
	handle_object() :
		fut_(),
		ctx_()
	{}

	handle_object(
			F const& fut,
			context const& ctx) :
		fut_( fut),
		ctx_( ctx)
	{}

	void interrupt()
	{ ctx_.interrupt(); }

	void interrupt_and_wait()
	{
		interrupt();
		wait();
	}

	bool interrupt_and_wait_until( system_time const& abs_time)
	{
		interrupt();
		return wait_until( abs_time);
	}

	bool interruption_requested()
	{ return ctx_.interruption_requested(); }

	R get()
	{
		try
		{ return fut_.get(); }
		catch ( future_uninitialized const&)
		{ throw task_uninitialized(); }
		catch ( broken_promise const&)
		{ throw broken_task(); }
		catch ( promise_already_satisfied const&)
		{ throw task_already_executed(); }
	}

	bool is_ready() const
	{ return fut_.is_ready(); }

	bool has_value() const
	{ return fut_.has_value(); }

	bool has_exception() const
	{ return fut_.has_exception(); }

	void wait() const
	{
		try
		{ fut_.wait(); }
		catch ( future_uninitialized const&)
		{ throw task_uninitialized(); }
		catch ( broken_promise const&)
		{ throw broken_task(); }
		catch ( thread_interrupted const&)
		{ throw task_interrupted(); }
	}

	bool wait_until( system_time const& abs_time) const
	{
		try
		{ return fut_.timed_wait_until( abs_time); }
		catch ( future_uninitialized const&)
		{ throw task_uninitialized(); }
		catch ( broken_promise const&)
		{ throw broken_task(); }
		catch ( thread_interrupted const&)
		{ throw task_interrupted(); }
	}
};

}

template< typename T >
struct is_handle_type;

template< typename R >
class handle
{
private:
	intrusive_ptr< detail::handle_base< R > >	base_;

public:
	handle() :
		base_( new detail::handle_object< R, spin::shared_future< R > >() )
	{}

	template< typename F >
	handle(
			F const& fut,
			context const& ctx) :
		base_( new detail::handle_object< R, F >( fut, ctx) )
	{}

	void interrupt()
	{ base_->interrupt(); }

	void interrupt_and_wait()
	{ base_->interrupt_and_wait(); }

	bool interrupt_and_wait_until( system_time const& abs_time)
	{ return base_->interrupt_and_wait_until( abs_time); }

	template< typename TimeDuration >
	bool interrupt_and_wait_for( TimeDuration const& rel_time)
	{ return interrupt_and_wait_until( get_system_time() + rel_time); }

	bool interruption_requested()
	{ return base_->interruption_requested(); }

	R get()
	{ return base_->get(); }

	bool is_ready() const
	{ return base_->is_ready(); }

	bool has_value() const
	{ return base_->has_value(); }

	bool has_exception() const
	{ return base_->has_exception(); }

	void wait() const
	{ base_->wait(); }

	bool wait_until( system_time const& abs_time) const
	{ return base_->wait_until( abs_time); }

	template< typename TimeDuration >
	bool wait_for( TimeDuration const& rel_time) const
	{ return wait_until( get_system_time() + rel_time); }

	void swap( handle< R > & other)
	{ base_.swap( other.base_); }
};

template< typename T >
struct is_handle_type
{ BOOST_STATIC_CONSTANT( bool, value = false); };

template< typename T >
struct is_handle_type< handle< T > >
{
    BOOST_STATIC_CONSTANT( bool, value = true);
};

template< typename Iterator >
typename disable_if< is_handle_type< Iterator >, void >::type waitfor_all(
	Iterator begin, Iterator end)
{
	for ( Iterator i = begin; i != end; ++i)
		i->wait();
}

template< typename R1, typename R2 >
void waitfor_all( handle< R1 > & h1, handle< R2 > & h2)
{
	h1.wait();
	h2.wait();
}

template< typename R1, typename R2, typename R3 >
void waitfor_all( handle< R1 > & h1, handle< R2 > & h2, handle< R3 > & h3)
{
	h1.wait();
	h2.wait();
	h3.wait();
}

template< typename R1, typename R2, typename R3, typename R4 >
void waitfor_all(
	handle< R1 > & h1, handle< R2 > & h2, handle< R3 > & h3, handle< R4 > & h4)
{
	h1.wait();
	h2.wait();
	h3.wait();
	h4.wait();
}

template< typename R1, typename R2, typename R3, typename R4, typename R5 >
void waitfor_all(
	handle< R1 > & h1, handle< R2 > & h2, handle< R3 > & h3, handle< R4 > & h4,
	handle< R5 > & h5)
{
	h1.wait();
	h2.wait();
	h3.wait();
	h4.wait();
	h5.wait();
}

}}

#include <boost/config/abi_suffix.hpp>

#endif // BOOST_TASKS_HANDLE_H
