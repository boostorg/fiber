
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TASKS_DETAIL_WORKER_H
#define BOOST_TASKS_DETAIL_WORKER_H

#include <cstddef>
#include <utility>

#include <boost/assert.hpp>
#include <boost/tasklet.hpp>
#include <boost/function.hpp>
#include <boost/random.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/utility.hpp>

#include <boost/task/callable.hpp>
#include <boost/task/detail/config.hpp>
#include <boost/task/detail/wsq.hpp>
#include <boost/task/poolsize.hpp>
#include <boost/task/stacksize.hpp>

#include <boost/config/abi_prefix.hpp>

# if defined(BOOST_MSVC)
# pragma warning(push)
# pragma warning(disable:4251 4275)
# endif

namespace boost {
namespace tasks {
namespace detail {

struct worker_base
{
	virtual ~worker_base() {}

	virtual const thread::id get_id() const = 0;

	virtual void join() const = 0;

	virtual void interrupt() const = 0;

	virtual void put( callable const&) = 0;

	virtual bool try_steal( callable &) = 0;

	virtual void run() = 0;

	virtual void yield() = 0;
};

template<
	typename Pool,
	typename Worker
>
class worker_object : public worker_base,
					  private noncopyable
{
private:
	class random_idx
	{
	private:
		rand48 rng_;
		uniform_int<> six_;
		variate_generator< rand48 &, uniform_int<> > die_;

	public:
		random_idx( std::size_t size) :
			rng_(),
			six_( 0, size - 1),
			die_( rng_, six_)
		{}

		std::size_t operator()()
		{ return die_(); }
	};

	typedef shared_ptr< thread >	thread_t;

	Pool						&	pool_;
	thread_t						thrd_;
	tasklets::scheduler<
		typename Pool::ums_type > *	sched_;// TODO: make not as pointer if WIN32 Fiber API is replaced by assembler

	wsq								wsq_;
	bool							shtdwn_;
	std::size_t						stack_size_;
	random_idx						rnd_idx_;

	void execute_( callable & ca)
	{
		BOOST_ASSERT( ! ca.empty() );
		{
			context_guard lk( ca, thrd_);
			ca();
		}
		ca.clear();
		BOOST_ASSERT( ca.empty() );
	}

	bool try_take_global_callable_( callable & ca)
	{ return pool_.queue_.try_take( ca); }

	bool try_take_local_callable_( callable & ca)
	{ return wsq_.try_take( ca); }
	
	bool try_steal_other_callable_( callable & ca)
	{
		std::size_t idx( rnd_idx_() );
		for ( std::size_t j( 0); j < pool_.wg_.size(); ++j)
		{
			Worker other( pool_.wg_[idx]);
			if ( this_thread::get_id() == other.get_id() ) continue;
			if ( ++idx >= pool_.wg_.size() ) idx = 0;
			if ( other.try_steal( ca) )
				return true;
		}
		return false;
	}

	void run_()
	{	
		while ( ! shutdown_() )
		{
			callable ca;
			if ( try_take_local_callable_( ca) || 
				 try_steal_other_callable_( ca) ||
				 try_take_global_callable_( ca) )
			{
				execute_( ca);
				if ( 0 < sched_->ready() ) return;
			}
			else
			{
				if ( 0 < sched_->ready() ) return;
				pool_.fsem_.wait();
			}
		}
	}

	bool shutdown_()
	{
		if ( shutdown__() && pool_.queue_.empty() && 0 == sched_->ready() )
			return true;
		else if ( shutdown_now__() )
			return true;
		return false;
	}

	bool shutdown__()
	{
		if ( ! shtdwn_)
			shtdwn_ = pool_.shtdwn_;
		return shtdwn_;
	}
	
	bool shutdown_now__()
	{ return pool_.shtdwn_now_; }

public:
	worker_object(
			Pool & pool,
			poolsize const& psize,
			stacksize const& stack_size,
			function< void() > const& fn) :
		pool_( pool),
		thrd_( new thread( fn) ),
		sched_(),
		wsq_( pool_.fsem_),
		shtdwn_( false),
		stack_size_( stack_size),
		rnd_idx_( psize)
	{ BOOST_ASSERT( ! fn.empty() ); }

	~worker_object()
	{ delete sched_; }

	const thread::id get_id() const
	{ return thrd_->get_id(); }

	void join() const
	{ thrd_->join(); }

	void
	interrupt() const
	{ thrd_->interrupt(); }

	void put( callable const& ca)
	{
		BOOST_ASSERT( ! ca.empty() );
		wsq_.put( ca);
	}

	bool try_steal( callable & ca)
	{ return wsq_.try_steal( ca); }

	void run()
	{
// TODO: remove if WIN32 Fiber API is replaced by assembler
		sched_ = new tasklets::scheduler< typename Pool::ums_type >();

		BOOST_ASSERT( get_id() == this_thread::get_id() );

		sched_->submit_tasklet(
				tasklets::make_tasklet(
					bind(
						& worker_object::run_,
						this),
					stack_size_) );
		while ( ! shutdown_() )
			sched_->run();	
	}

	void yield()
	{
		sched_->submit_tasklet(
			tasklets::make_tasklet(
				bind(
					& worker_object::run_,
					this),
				stack_size_) );
		this_tasklet::yield();
	}
};

class BOOST_TASKS_DECL worker
{
private:
	static thread_specific_ptr< worker >	tss_;

	shared_ptr< worker_base >	impl_;

public:
	template< typename Pool >
	worker(
			Pool & pool,
			poolsize const& psize,
			stacksize const& stack_size,
			function< void() > const& fn) :
		impl_(
			new worker_object< Pool, worker >(
				pool,
				psize,
				stack_size,
				fn) )
	{}

	const thread::id get_id() const;

	void join() const;

	void interrupt() const;

	void put( callable const&);

	bool try_steal( callable &);

	void run();

	void yield();

	static worker * tss_get();
};

}}}

# if defined(BOOST_MSVC)
# pragma warning(pop)
# endif

#include <boost/config/abi_suffix.hpp>

#endif // BOOST_TASKS_DETAIL_WORKER_H

