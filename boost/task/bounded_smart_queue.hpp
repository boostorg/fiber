
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TASKS_BOUNDED_SMART_QUEUE_H
#define BOOST_TASKS_BOUNDED_SMART_QUEUE_H

#include <algorithm>
#include <cstddef>

#include <boost/assert.hpp>
#include <boost/atomic.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/shared_mutex.hpp>

#include <boost/task/callable.hpp>
#include <boost/task/detail/meta.hpp>
#include <boost/task/detail/smart.hpp>
#include <boost/task/exceptions.hpp>
#include <boost/task/fast_semaphore.hpp>
#include <boost/task/watermark.hpp>

#include <boost/config/abi_prefix.hpp>

namespace boost {
namespace tasks {

template<
	typename Attr,
	typename Comp,
	typename Enq = detail::replace_oldest,
	typename Deq = detail::take_oldest
>
class bounded_smart_queue
{
public:
	typedef detail::has_attribute	attribute_tag_type;
	typedef Attr					attribute_type;

	struct value_type
	{
		callable		ca;
		attribute_type	attr;

		value_type(
				callable const& ca_,
				attribute_type const& attr_) :
			ca( ca_), attr( attr_)
		{ BOOST_ASSERT( ! ca.empty() ); }

		void swap( value_type & other)
		{
			ca.swap( other.ca);
			std::swap( attr, other.attr);
		}
	};

private:
	typedef multi_index::multi_index_container<
		value_type,
		multi_index::indexed_by<
			multi_index::ordered_non_unique<
				multi_index::member<
					value_type,
					Attr,
					& value_type::attr
				>,
				Comp
			>
		>
	>															queue_type;
	typedef typename queue_type::template nth_index< 0 >::type	queue_index;

	enum state
	{
		ACTIVE = 0,
		DEACTIVE
	};

	atomic< state >			state_;
	queue_type				queue_;
	queue_index	&			idx_;
	mutable shared_mutex	mtx_;
	condition				not_full_cond_;
	Enq						enq_op_;
	Deq						deq_op_;
	std::size_t				hwm_;
	std::size_t				lwm_;
	fast_semaphore		&	fsem_;

	bool active_() const
	{ return ACTIVE == state_.load(); }

	void deactivate_()
	{ state_.store( DEACTIVE); }

	bool empty_() const
	{ return queue_.empty(); }

	bool full_() const
	{ return size_() >= hwm_; }

	std::size_t size_() const
	{ return queue_.size(); }

	void put_(
		value_type const& va,
		unique_lock< shared_mutex > & lk)
	{
		if ( full_() )
		{
			not_full_cond_.wait(
				lk,
				bind(
					& bounded_smart_queue::producers_activate_,
					this) );
		}
		if ( ! active_() )
			throw task_rejected("queue is not active");
		enq_op_( idx_, va);
		fsem_.post();
	}

	template< typename TimeDuration >
	void put_(
		value_type const& va,
		TimeDuration const& rel_time,
		unique_lock< shared_mutex > & lk)
	{
		if ( full_() )
		{
			if ( ! not_full_cond_.timed_wait(
				lk,
				rel_time,
				bind(
					& bounded_smart_queue::producers_activate_,
					this) ) )
				throw task_rejected("timed out");
		}
		if ( ! active_() )
			throw task_rejected("queue is not active");
		enq_op_( idx_, va);
		fsem_.post();
	}

	bool try_take_( callable & ca)
	{
		if ( empty_() )
			return false;
		deq_op_( idx_, ca);
		bool valid = ! ca.empty();
		if ( valid && size_() <= lwm_)
		{
			if ( lwm_ == hwm_)
				not_full_cond_.notify_one();
			else
				// more than one producer could be waiting
				// in order to submit an task
				not_full_cond_.notify_all();
		}
		return valid;
	}

	bool producers_activate_() const
	{ return ! active_() || ! full_(); }

public:
	bounded_smart_queue(
			fast_semaphore & fsem,
			high_watermark const& hwm,
			low_watermark const& lwm) :
		state_( ACTIVE),
		queue_(),
		idx_( queue_.get< 0 >() ),
		mtx_(),
		not_full_cond_(),
		enq_op_(),
		deq_op_(),
		hwm_( hwm),
		lwm_( lwm),
		fsem_( fsem)
	{
		if ( lwm_ > hwm_ )
			throw invalid_watermark();
	}

	bool active() const
	{ return active_(); }

	void deactivate()
	{
		unique_lock< shared_mutex > lk( mtx_);
		deactivate_();
		not_full_cond_.notify_all();
	}

	bool empty() const
	{
		shared_lock< shared_mutex > lk( mtx_);
		return empty_();
	}

	std::size_t upper_bound() const
	{
		shared_lock< shared_mutex > lk( mtx_);
		return hwm_;
	}

	std::size_t lower_bound() const
	{
		shared_lock< shared_mutex > lk( mtx_);
		return lwm_;
	}

	void put( value_type const& va)
	{
		unique_lock< shared_mutex > lk( mtx_);
		put_( va, lk);
	}

	template< typename TimeDuration >
	void put(
		value_type const& va,
		TimeDuration const& rel_time)
	{
		unique_lock< shared_mutex > lk( mtx_);
		put_( va, rel_time, lk);
	}

	bool try_take( callable & ca)
	{
		unique_lock< shared_mutex > lk( mtx_);
		return try_take_( ca);
	}
};

}}

#include <boost/config/abi_suffix.hpp>

#endif // BOOST_TASKS_BOUNDED_SMART_QUEUE_H
