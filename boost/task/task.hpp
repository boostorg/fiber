
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TASKS_TASK_H
#define BOOST_TASKS_TASK_H

#include <boost/atomic.hpp>
#include <boost/bind.hpp>
#include <boost/config.hpp>
#include <boost/exception_ptr.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/move/move.hpp>
#include <boost/preprocessor/repetition.hpp>
#include <boost/thread.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/utility/result_of.hpp>

#include <boost/task/exceptions.hpp>
#include <boost/task/spin/future.hpp>

#include <boost/config/abi_prefix.hpp>

namespace boost {
namespace tasks {
namespace detail {

template< typename R >
struct promise_adaptor
{
	template< typename X >
	friend void intrusive_ptr_add_ref( promise_adaptor< X > *);
	template< typename X >
	friend void intrusive_ptr_release( promise_adaptor< X > *);

	typedef intrusive_ptr< promise_adaptor >	ptr;

	atomic< unsigned int >	use_count;

	virtual ~promise_adaptor() {}

	virtual void set_value(
		typename tasks::detail::future_traits< R >::source_reference_type) = 0;

	virtual void set_value(
		typename tasks::detail::future_traits< R >::rvalue_source_type) = 0;

	virtual void set_exception( exception_ptr) = 0;
};

template<>
struct promise_adaptor< void >
{
	template< typename X >
	friend void intrusive_ptr_add_ref( promise_adaptor< X > *);
	template< typename X >
	friend void intrusive_ptr_release( promise_adaptor< X > *);

	typedef intrusive_ptr< promise_adaptor >	ptr;

	atomic< unsigned int >	use_count;

	virtual ~promise_adaptor() {}

	virtual void set_value() = 0;

	virtual void set_exception( exception_ptr) = 0;
};

template< typename R >
void intrusive_ptr_add_ref( promise_adaptor< R > * p)
{ p->use_count.fetch_add( 1, memory_order_relaxed); }

template< typename R >
void intrusive_ptr_release( promise_adaptor< R > * p)
{
	if ( p->use_count.fetch_sub( 1, memory_order_release) == 1)
	{
		atomic_thread_fence( memory_order_acquire);
		delete p;
	}
}

template< typename R, template< typename > class Promise >
class promise_wrapper;

template< typename R >
class promise_wrapper< R, promise > : public promise_adaptor< R >
{
private:
	promise< R >	prom_;

public:	
#ifdef BOOST_HAS_RVALUE_REFS
	promise_wrapper( promise< R > && prom) :
		prom_( prom)
	{}
#else
	promise_wrapper( boost::detail::thread_move_t< promise< R > > prom) :
		prom_( prom)
	{}
#endif

	void set_value(
		typename tasks::detail::future_traits< R >::source_reference_type r)
	{ prom_.set_value( r); };

	void set_value(
		typename tasks::detail::future_traits< R >::rvalue_source_type r)
	{ prom_.set_value( r); };

	void set_exception( exception_ptr p)
	{ prom_.set_exception( p); }
};

template<>
class promise_wrapper< void, promise > : public promise_adaptor< void >
{
private:
	promise< void >	prom_;

public:	
#ifdef BOOST_HAS_RVALUE_REFS
	promise_wrapper( promise< void > && prom) :
		prom_( prom)
	{}
#else
	promise_wrapper( boost::detail::thread_move_t< promise< void > > prom) :
		prom_( prom)
	{}
#endif

	void set_value()
	{ prom_.set_value(); };

	void set_exception( exception_ptr p)
	{ prom_.set_exception( p); }
};

template< typename R >
class promise_wrapper< R, spin::promise > : public promise_adaptor< R >
{
private:
	spin::promise< R >	prom_;

public:	
	promise_wrapper() :
		prom_()
	{}

	promise_wrapper( BOOST_RV_REF( spin::promise< R >) prom) :
		prom_( prom)
	{}

	void set_value(
		typename tasks::detail::future_traits< R >::source_reference_type r)
	{ prom_.set_value( r); };

	void set_value(
		typename tasks::detail::future_traits< R >::rvalue_source_type r)
	{ prom_.set_value( r); };

	void set_exception( exception_ptr p)
	{ prom_.set_exception( p); }
};

template<>
class promise_wrapper< void, spin::promise > : public promise_adaptor< void >
{
private:
	spin::promise< void >	prom_;

public:
	promise_wrapper() :
		prom_()
	{}

	promise_wrapper( BOOST_RV_REF( spin::promise< void >) prom) :
		prom_( prom)
	{}

	void set_value()
	{ prom_.set_value(); };

	void set_exception( exception_ptr p)
	{ prom_.set_exception( p); }
};

template< typename R >
class task_base
{
private:
	template< typename X >
	friend void intrusive_ptr_add_ref( task_base< X > *);
	template< typename X >
	friend void intrusive_ptr_release( task_base< X > *);

	atomic< unsigned int >	use_count_;

protected:
	bool								done_;
	typename promise_adaptor< R >::ptr	prom_;

	virtual void do_run() = 0;

public:
	task_base() :
		done_( false),
		prom_( new promise_wrapper< R, spin::promise >() )
	{}

	virtual ~task_base() {}

	void run()
	{
		if ( this->done_) throw task_already_executed();
		do_run();
		done_ = true;
	}

	void set_promise( typename promise_adaptor< R >::ptr prom)
	{ prom_ = prom; }
};

template< typename R >
void intrusive_ptr_add_ref( task_base< R > * p)
{ p->use_count_.fetch_add( 1, memory_order_relaxed); }

template< typename R >
void intrusive_ptr_release( task_base< R > * p)
{
	if ( p->use_count_.fetch_sub( 1, memory_order_release) == 1)
	{
		atomic_thread_fence( memory_order_acquire);
		delete p;
	}
}

template< typename R, typename Fn >
class task_wrapper : public task_base< R >
{
private:
	Fn		fn_;

	void do_run()
	{
		try
		{ this->prom_->set_value( fn_() ); }
		catch ( promise_already_satisfied const&)
		{ throw task_already_executed(); }
		catch ( thread_interrupted const&)
		{ this->prom_->set_exception( copy_exception( task_interrupted() ) ); }
		catch ( task_interrupted const& e)
		{ this->prom_->set_exception( copy_exception( e) ); }
		catch ( boost::exception const& e)
		{ this->prom_->set_exception( copy_exception( e) ); }
		catch ( std::ios_base::failure const& e)
		{ this->prom_->set_exception( copy_exception( e) ); }
		catch ( std::domain_error const& e)
		{ this->prom_->set_exception( copy_exception( e) ); }
		catch ( std::invalid_argument const& e)
		{ this->prom_->set_exception( copy_exception( e) ); }
		catch ( std::length_error const& e)
		{ this->prom_->set_exception( copy_exception( e) ); }
		catch ( std::out_of_range const& e)
		{ this->prom_->set_exception( copy_exception( e) ); }
		catch ( std::logic_error const& e)
		{ this->prom_->set_exception( copy_exception( e) ); }
		catch ( std::overflow_error const& e)
		{ this->prom_->set_exception( copy_exception( e) ); }
		catch ( std::range_error const& e)
		{ this->prom_->set_exception( copy_exception( e) ); }
		catch ( std::underflow_error const& e)
		{ this->prom_->set_exception( copy_exception( e) ); }
		catch ( std::runtime_error const& e)
		{ this->prom_->set_exception( copy_exception( e) ); }
		catch ( std::bad_alloc const& e)
		{ this->prom_->set_exception( copy_exception( e) ); }
		catch ( std::bad_cast const& e)
		{ this->prom_->set_exception( copy_exception( e) ); }
		catch ( std::bad_typeid const& e)
		{ this->prom_->set_exception( copy_exception( e) ); }
		catch ( std::bad_exception const& e)
		{ this->prom_->set_exception( copy_exception( e) ); }
		catch (...)
		{ this->prom_->set_exception( current_exception() ); }
	}

public:
	task_wrapper( Fn const& fn) :
		task_base< R >(), fn_( fn)
	{}
};

template< typename Fn >
class task_wrapper< void, Fn > : public task_base< void >
{
private:
	Fn		fn_;

	void do_run()
	{
		try
		{
			fn_();
			this->prom_->set_value();
		}
		catch ( promise_already_satisfied const&)
		{ throw task_already_executed(); }
		catch ( thread_interrupted const&)
		{ this->prom_->set_exception( copy_exception( task_interrupted() ) ); }
		catch ( task_interrupted const& e)
		{ this->prom_->set_exception( copy_exception( e) ); }
		catch ( boost::exception const& e)
		{ this->prom_->set_exception( copy_exception( e) ); }
		catch ( std::ios_base::failure const& e)
		{ this->prom_->set_exception( copy_exception( e) ); }
		catch ( std::domain_error const& e)
		{ this->prom_->set_exception( copy_exception( e) ); }
		catch ( std::invalid_argument const& e)
		{ this->prom_->set_exception( copy_exception( e) ); }
		catch ( std::length_error const& e)
		{ this->prom_->set_exception( copy_exception( e) ); }
		catch ( std::out_of_range const& e)
		{ this->prom_->set_exception( copy_exception( e) ); }
		catch ( std::logic_error const& e)
		{ this->prom_->set_exception( copy_exception( e) ); }
		catch ( std::overflow_error const& e)
		{ this->prom_->set_exception( copy_exception( e) ); }
		catch ( std::range_error const& e)
		{ this->prom_->set_exception( copy_exception( e) ); }
		catch ( std::underflow_error const& e)
		{ this->prom_->set_exception( copy_exception( e) ); }
		catch ( std::runtime_error const& e)
		{ this->prom_->set_exception( copy_exception( e) ); }
		catch ( std::bad_alloc const& e)
		{ this->prom_->set_exception( copy_exception( e) ); }
		catch ( std::bad_cast const& e)
		{ this->prom_->set_exception( copy_exception( e) ); }
		catch ( std::bad_typeid const& e)
		{ this->prom_->set_exception( copy_exception( e) ); }
		catch ( std::bad_exception const& e)
		{ this->prom_->set_exception( copy_exception( e) ); }
		catch (...)
		{ this->prom_->set_exception( current_exception() ); }
	}

public:
	task_wrapper( Fn const& fn) :
		task_base< void >(), fn_( fn)
	{}
};

}

template< typename R >
class task
{
private:
	template< typename T, typename P >
	friend class detail::callable_object;

	BOOST_MOVABLE_BUT_NOT_COPYABLE( task);	

	intrusive_ptr< detail::task_base< R > >	task_;

// TODO: if boost.thread uses boost.move
// re-work set_promise
#ifdef BOOST_HAS_RVALUE_REFS
	void set_promise( promise< R > && prom)
	{
		if ( ! task_) throw task_moved();
		task_->set_promise(
			new detail::promise_wrapper< R, promise >( prom) );	
	}
#else
	void set_promise( boost::detail::thread_move_t< promise< R > > prom)
	{
		if ( ! task_) throw task_moved();
		task_->set_promise(
			new detail::promise_wrapper< R, promise >( prom) );	
	}
#endif

	void set_promise( BOOST_RV_REF( spin::promise< R >) prom)
	{
		if ( ! task_) throw task_moved();
		task_->set_promise(
			new detail::promise_wrapper< R, spin::promise >( prom) );	
	}

public:
	task() :
		task_()
	{}

	explicit task( R( * fn)()) :
		task_( new detail::task_wrapper< R, R( *)() >( fn) )
	{}

	template< typename Fn >
	explicit task( BOOST_RV_REF( Fn) fn) :
		task_( new detail::task_wrapper< R, Fn >( fn) )
	{}

	task( BOOST_RV_REF( task) other) :
		task_()
	{ task_.swap( other.task_); }

	task & operator=( BOOST_RV_REF( task) other)
	{
		task tmp( other);
		swap( tmp);
		return * this;
	}

# ifndef BOOST_TASKS_MAX_ARITY
#   define BOOST_TASKS_MAX_ARITY 10
# endif

# define BOOST_TASKS_ARG(z, n, unused) \
   BOOST_PP_CAT(A, n) BOOST_PP_CAT(a, n)
# define BOOST_ENUM_TASK_ARGS(n) BOOST_PP_ENUM(n, BOOST_TASKS_ARG, ~)

# define BOOST_TASKS_CTOR(z, n, unused)	\
template< \
	typename Fn, \
	BOOST_PP_ENUM_PARAMS(n, typename A)	\
>										\
explicit task( Fn fn, BOOST_ENUM_TASK_ARGS(n))	\
	: task_(									\
			new detail::task_wrapper< R, function< R() > >(	\
				bind( fn, BOOST_PP_ENUM_PARAMS(n, a)) ) )	\
	{}

BOOST_PP_REPEAT_FROM_TO( 1, BOOST_TASKS_MAX_ARITY, BOOST_TASKS_CTOR, ~)

# undef BOOST_TASKS_CTOR

	void operator()()
	{
		if ( ! task_) throw task_moved();
		task_->run();
	}

	typedef typename shared_ptr< detail::task_base< R > >::unspecified_bool_type
		unspecified_bool_type;

	operator unspecified_bool_type() const // throw()
	{ return task_; }

	bool operator!() const // throw()
	{ return ! task_; }

	void swap( task & other) // throw()
	{ task_.swap( other.task_); }
};

template< typename Fn >
task< typename result_of< Fn() >::type > make_task( Fn fn)
{ return task< typename boost::result_of< Fn() >::type >( fn); }

# define BOOST_TASKS_MAKE_TASK_FUNCTION(z, n, unused)	\
template<												\
	typename Fn,										\
	BOOST_PP_ENUM_PARAMS(n, typename A)					\
>														\
task< typename result_of< Fn( BOOST_PP_ENUM_PARAMS(n, A)) >::type >		\
make_task( Fn fn, BOOST_ENUM_TASK_ARGS(n))				\
{ \
	return task< \
		typename result_of< Fn( BOOST_PP_ENUM_PARAMS(n, A)) >::type >( \
			fn, BOOST_PP_ENUM_PARAMS(n, a)); \
}

BOOST_PP_REPEAT_FROM_TO(1,BOOST_TASKS_MAX_ARITY,BOOST_TASKS_MAKE_TASK_FUNCTION, ~)

# undef BOOST_TASKS_MAKE_TASK_FUNCTION
# undef BOOST_ENUM_TASK_ARGS
# undef BOOST_TASKS_ARG
# undef BOOST_TASKS_MAX_ARITY

}

using tasks::task;

template< typename R >
void swap( task< R > & l, task< R > & r)
{ return l.swap( r); }

}

#include <boost/config/abi_suffix.hpp>

#endif // BOOST_TASKS_TASK_H
