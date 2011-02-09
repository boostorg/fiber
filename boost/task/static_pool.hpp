
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TASKS_STATIC_POOL_H
#define BOOST_TASKS_STATIC_POOL_H

#include <cstddef>

#include <boost/config.hpp>
#include <boost/tasklet/tasklet.hpp>
#include <boost/tasklet/round_robin.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/move/move.hpp>

#include <boost/task/detail/bind_processor.hpp>
#include <boost/task/detail/pool_base.hpp>
#include <boost/task/detail/worker_group.hpp>
#include <boost/task/exceptions.hpp>
#include <boost/task/handle.hpp>
#include <boost/task/poolsize.hpp>
#include <boost/task/stacksize.hpp>
#include <boost/task/task.hpp>
#include <boost/task/watermark.hpp>

#include <boost/config/abi_prefix.hpp>

namespace boost {
namespace tasks {

template<
	typename Queue,
	typename UMS = tasklets::round_robin
>
class static_pool
{
public:
	typedef Queue	queue_type;
	typedef UMS		ums_type;

private:
	typedef detail::pool_base< queue_type, ums_type >	base_type;

	template< typename T, typename X >
	friend class detail::worker_object;

	BOOST_MOVABLE_BUT_NOT_COPYABLE( static_pool);	

# if defined(BOOST_HAS_PROCESSOR_BINDINGS)
	struct tag_bind_to_processors {};
# endif
	
	intrusive_ptr< base_type >		pool_;

public:
	static_pool() :
		pool_()
	{}
	
	explicit static_pool(
			poolsize const& psize,
			stacksize const& stack_size = stacksize( tasklet::default_stacksize) ) :
		pool_( new base_type( psize, stack_size) )
	{}

	explicit static_pool(
			poolsize const& psize,
			high_watermark const& hwm,
			low_watermark const& lwm,
			stacksize const& stack_size = stacksize( tasklet::default_stacksize) ) :
		pool_( new base_type( psize, hwm, lwm, stack_size) )
	{}

# if defined(BOOST_HAS_PROCESSOR_BINDINGS)
	explicit static_pool(
			tag_bind_to_processors,
			stacksize const& stack_size = stacksize( tasklet::default_stacksize) ) :
		pool_( new base_type( stack_size) )
	{}

	explicit static_pool(
			tag_bind_to_processors,
			high_watermark const& hwm,
			low_watermark const& lwm,
			stacksize const& stack_size = stacksize( tasklet::default_stacksize) ) :
		pool_( new base_type( hwm, lwm, stack_size) )
	{}

	static tag_bind_to_processors bind_to_processors()
	{ return tag_bind_to_processors(); }
# endif

	static_pool( BOOST_RV_REF( static_pool) other) :
		pool_()
	{ pool_.swap( other.pool_); }

	static_pool & operator=( BOOST_RV_REF( static_pool) other)
	{
		static_pool tmp( other);
		swap( tmp);
		return * this;
	}

	void interrupt_all_worker()
	{
		if ( ! pool_)
			throw pool_moved();
		pool_->interrupt_all_worker();
	}

	void shutdown()
	{
		if ( ! pool_)
			throw pool_moved();
		pool_->shutdown();
	}

	const void shutdown_now()
	{
		if ( ! pool_)
			throw pool_moved();
		pool_->shutdown_now();
	}

	std::size_t size()
	{
		if ( ! pool_)
			throw pool_moved();
		return pool_->size();
	}

	bool closed()
	{
		if ( ! pool_)
			throw pool_moved();
		return pool_->closed();
	}

	std::size_t upper_bound()
	{
		if ( ! pool_)
			throw pool_moved();
		return pool_->upper_bound();
	}

	void upper_bound( high_watermark const& hwm)
	{
		if ( ! pool_)
			throw pool_moved();
		pool_->upper_bound( hwm);
	}

	std::size_t lower_bound()
	{
		if ( ! pool_)
			throw pool_moved();
		return pool_->lower_bound();
	}

	void lower_bound( low_watermark const lwm)
	{
		if ( ! pool_)
			throw pool_moved();
		pool_->lower_bound( lwm);
	}

	template< typename R >
	handle< R > submit( task< R > t)
	{
		if ( ! pool_)
			throw pool_moved();
		return pool_->submit( boost::move( t) );
	}

	template< typename R, typename Attr >
	handle< R > submit( task< R > t, Attr const& attr)
	{
		if ( ! pool_)
			throw pool_moved();
		return pool_->submit( boost::move( t), attr);
	}

	typedef typename shared_ptr< base_type >::unspecified_bool_type	unspecified_bool_type;

	operator unspecified_bool_type() const // throw()
	{ return pool_; }

	bool operator!() const // throw()
	{ return ! pool_; }

	void swap( static_pool & other) // throw()
	{ pool_.swap( other.pool_); }
};

}

template< typename Queue, typename UMS >
void swap( tasks::static_pool< Queue, UMS > & l, tasks::static_pool< Queue, UMS > & r)
{ return l.swap( r); }

}

#include <boost/config/abi_suffix.hpp>

#endif // BOOST_TASKS_STATIC_POOL_H

