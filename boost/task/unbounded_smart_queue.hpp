
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TASKS_UNBOUNDED_SMART_QUEUE_H
#define BOOST_TASKS_UNBOUNDED_SMART_QUEUE_H

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

#include <boost/config/abi_prefix.hpp>

namespace boost {
namespace tasks {

template<
	typename Attr,
	typename Comp,
	typename Enq = detail::replace_oldest,
	typename Deq = detail::take_oldest
>
class unbounded_smart_queue
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
	Enq						enq_op_;
	Deq						deq_op_;
	fast_semaphore		&	fsem_;

	bool active_() const
	{ return ACTIVE == state_.load(); }

	void deactivate_()
	{ state_.store( DEACTIVE); }

	bool empty_() const
	{ return queue_.empty(); }

	void put_( value_type const& va)
	{
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
		return ! ca.empty();
	}

public:
	unbounded_smart_queue( fast_semaphore & fsem) :
		state_( ACTIVE),
		queue_(),
		idx_( queue_.get< 0 >() ),
		mtx_(),
		enq_op_(),
		deq_op_(),
		fsem_( fsem)
	{}

	bool active() const
	{ return active_(); }

	void deactivate()
	{
		unique_lock< shared_mutex > lk( mtx_);
		deactivate_();
	}

	bool empty() const
	{
		shared_lock< shared_mutex > lk( mtx_);
		return empty_();	
	}

	void put( value_type const& va)
	{
		unique_lock< shared_mutex > lk( mtx_);
		put_( va);
	}

	bool try_take( callable & ca)
	{
		unique_lock< shared_mutex > lk( mtx_);
		return try_take_( ca);
	}
};

}}

#include <boost/config/abi_suffix.hpp>

#endif // BOOST_TASKS_UNBOUNDED_SMART_QUEUE_H
