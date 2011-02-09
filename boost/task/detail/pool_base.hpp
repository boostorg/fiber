
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TASKS_DETAIL_POOL_BASE_H
#define BOOST_TASKS_DETAIL_POOL_BASE_H

#include <cstddef>

#include <boost/assert.hpp>
#include <boost/atomic.hpp>
#include <boost/bind.hpp>
#include <boost/config.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/thread/detail/move.hpp>
#include <boost/thread/future.hpp>

#include <boost/task/callable.hpp>
#include <boost/task/context.hpp>
#include <boost/task/detail/bind_processor.hpp>
#include <boost/task/detail/worker_group.hpp>
#include <boost/task/detail/worker.hpp>
#include <boost/task/exceptions.hpp>
#include <boost/task/fast_semaphore.hpp>
#include <boost/task/handle.hpp>
#include <boost/task/poolsize.hpp>
#include <boost/task/spin/future.hpp>
#include <boost/task/stacksize.hpp>
#include <boost/task/task.hpp>
#include <boost/task/utility.hpp>
#include <boost/task/watermark.hpp>

#include <boost/config/abi_prefix.hpp>

namespace boost {
namespace tasks {
namespace detail {

template<
	typename Queue,
	typename UMS
>
class pool_base
{
private:
	friend class worker;
	template< typename T, typename Z >
	friend class worker_object;
	template< typename T, typename Z >
	friend void intrusive_ptr_add_ref( pool_base< T, Z > * p);
	template< typename T, typename Z >
	friend void intrusive_ptr_release( pool_base< T, Z > * p);

	typedef Queue							queue_type;
	typedef typename queue_type::value_type	value_type;
	typedef UMS								ums_type;

	enum state
	{
		ACTIVE = 0,
		DEACTIVE	
	};

	fast_semaphore			fsem_;
	atomic< unsigned int >	use_count_;
	worker_group			wg_;
	shared_mutex			mtx_wg_;
	atomic< state >			state_;
	queue_type				queue_;
	atomic< bool >			shtdwn_;
	atomic< bool >			shtdwn_now_;

	void worker_entry_()
	{
		shared_lock< shared_mutex > lk( mtx_wg_);
		typename detail::worker_group::iterator i( wg_.find( this_thread::get_id() ) );
		lk.unlock();
		BOOST_ASSERT( i != wg_.end() );

		worker w( * i);
		w.run();
	}

	void create_worker_(
		poolsize const& psize,
		stacksize const& stack_size)
	{
		wg_.insert(
			worker(
				* this,
				psize,
				stack_size,
				boost::bind(
					& pool_base::worker_entry_,
					this) ) );
	}

# if defined(BOOST_HAS_PROCESSOR_BINDINGS)
	void worker_entry_( std::size_t n)
	{
		this_thread::bind_to_processor( n);
		worker_entry_();
	}

	void create_worker_(
		poolsize const& psize,
		stacksize const& stack_size,
		std::size_t n)
	{
		wg_.insert(
			worker(
				* this,
				psize,
				stack_size,
				boost::bind(
					& pool_base::worker_entry_,
					this,
					n) ) );
	}
# endif

	std::size_t size_() const
	{ return wg_.size(); }

	bool deactivated_() const
	{ return DEACTIVE == state_.load(); }

	bool deactivate_()
	{ return ACTIVE == state_.exchange( DEACTIVE); }

public:
	explicit pool_base(
			poolsize const& psize,
			stacksize const& stack_size) :
		fsem_( 0),
		use_count_( 0),
		wg_(),
		mtx_wg_(),
		state_( ACTIVE),
		queue_( fsem_),
		shtdwn_( false),
		shtdwn_now_( false)
	{
		lock_guard< shared_mutex > lk( mtx_wg_);
		for ( std::size_t i( 0); i < psize; ++i)
			create_worker_( psize, stack_size);
	}

	explicit pool_base(
			poolsize const& psize,
			high_watermark const& hwm,
			low_watermark const& lwm,
			stacksize const& stack_size) :
		fsem_( 0),
		use_count_( 0),
		wg_(),
		mtx_wg_(),
		state_( ACTIVE),
		queue_( fsem_, hwm, lwm),
		shtdwn_( false),
		shtdwn_now_( false)
	{
		lock_guard< shared_mutex > lk( mtx_wg_);
		for ( std::size_t i( 0); i < psize; ++i)
			create_worker_( psize, stack_size);
	}

# if defined(BOOST_HAS_PROCESSOR_BINDINGS)
	explicit pool_base( stacksize const& stack_size) :
		fsem_( 0),
		use_count_( 0),
		wg_(),
		mtx_wg_(),
		state_( ACTIVE),
		queue_( fsem_),
		shtdwn_( false),
		shtdwn_now_( false)
	{
		poolsize psize( thread::hardware_concurrency() );
		BOOST_ASSERT( psize > 0);
		lock_guard< shared_mutex > lk( mtx_wg_);
		for ( std::size_t i( 0); i < psize; ++i)
			create_worker_( psize, stack_size, i);
	}

	explicit pool_base(
			high_watermark const& hwm,
			low_watermark const& lwm,
			stacksize const& stack_size) :
		fsem_( 0),
		use_count_( 0),
		wg_(),
		mtx_wg_(),
		state_( ACTIVE),
		queue_( fsem_, hwm, lwm),
		shtdwn_( false),
		shtdwn_now_( false)
	{
		poolsize psize( thread::hardware_concurrency() );
		BOOST_ASSERT( psize > 0);
		lock_guard< shared_mutex > lk( mtx_wg_);
		for ( std::size_t i( 0); i < psize; ++i)
			create_worker_( psize, stack_size, i);
	}
# endif

	~pool_base()
	{ shutdown(); }

	void interrupt_all_worker()
	{
		if ( deactivated_() ) return;

		shared_lock< shared_mutex > lk( mtx_wg_);
		wg_.interrupt_all();
	}

	void shutdown()
	{
		if ( deactivated_() || ! deactivate_() ) return;

		queue_.deactivate();
		fsem_.deactivate();
		shared_lock< shared_mutex > lk( mtx_wg_);
		shtdwn_.store( true);
		wg_.join_all();
	}

	const void shutdown_now()
	{
		if ( deactivated_() || ! deactivate_() ) return;

		queue_.deactivate();
		fsem_.deactivate();
		shared_lock< shared_mutex > lk( mtx_wg_);
		shtdwn_now_.store( true);
		wg_.interrupt_all();
		wg_.join_all();
	}

	std::size_t size()
	{
		shared_lock< shared_mutex > lk( mtx_wg_);
		return size_();
	}

	bool closed()
	{ return deactivated_(); }

	std::size_t upper_bound()
	{ return queue_.upper_bound(); }

	void upper_bound( high_watermark const& hwm)
	{ queue_.upper_bound( hwm); }

	std::size_t lower_bound()
	{ return queue_.lower_bound(); }

	void lower_bound( low_watermark const lwm)
	{ queue_.lower_bound( lwm); }

	template< typename R >
	handle< R > submit( BOOST_RV_REF( task< R >) t)
	{
		if ( deactivated_() )
			throw task_rejected("pool is closed");

		if ( this_task::runs_in_pool() )
		{
			spin::promise< R > prom;
			spin::shared_future< R > f( prom.get_future() );
			context ctx;
			handle< R > h( f, ctx);
			queue_.put( callable( t, boost::move( prom), ctx) );
			return h;
		}
		else
		{
			promise< R > prom;
			shared_future< R > f( prom.get_future() );
			context ctx;
			handle< R > h( f, ctx);
			queue_.put(
					callable(
							t,
// TODO: workaround because thread_move_t will be abigous for move
#ifdef BOOST_HAS_RVALUE_REFS
							boost::move( prom),
#else
							boost::detail::thread_move_t< promise< R > >( prom),
#endif
							ctx) );
			return h;
		}
	}

	template< typename R, typename Attr >
	handle< R > submit( BOOST_RV_REF( task< R >) t, Attr const& attr)
	{
		if ( deactivated_() )
			throw task_rejected("pool is closed");

		if ( this_task::runs_in_pool() )
		{
			spin::promise< R > prom;
			spin::shared_future< R > f( prom.get_future() );
			context ctx;
			handle< R > h( f, ctx);
			queue_.put(
				value_type(
					callable( t, boost::move( prom), ctx),
					attr) );
			return h;
		}
		else
		{
			promise< R > prom;
			shared_future< R > f( prom.get_future() );
			// TODO: if boost.thread uses boost.move
			// use boost::move()
			context ctx;
			handle< R > h( f, ctx);
			queue_.put(
				value_type(
					callable(
							t,
// TODO: workaround because thread_move_t will be abigous for move
#ifdef BOOST_HAS_RVALUE_REFS
							boost::move( prom),
#else
							boost::detail::thread_move_t< promise< R > >( prom),
#endif
							ctx),
					attr) );
			return h;
		}
	}
};

template<
	typename Queue,
	typename UMS
>
void intrusive_ptr_add_ref( pool_base< Queue, UMS > * p)
{ p->use_count_.fetch_add( 1, memory_order_relaxed); }

template<
	typename Queue,
	typename UMS
>
void intrusive_ptr_release( pool_base< Queue, UMS > * p)
{
	if ( p->use_count_.fetch_sub( 1, memory_order_release) == 1)
	{
		atomic_thread_fence( memory_order_acquire);
		delete p;
	}
}

}}}

#include <boost/config/abi_suffix.hpp>

#endif // BOOST_TASKS_DETAIL_POOL_BASE_H

