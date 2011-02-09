
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TASKS_BOUNDED_PRIO_QUEUE_H
#define BOOST_TASKS_BOUNDED_PRIO_QUEUE_H

#include <algorithm>
#include <cstddef>
#include <functional>
#include <queue>

#include <boost/assert.hpp>
#include <boost/atomic.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/shared_mutex.hpp>

#include <boost/task/callable.hpp>
#include <boost/task/detail/meta.hpp>
#include <boost/task/exceptions.hpp>
#include <boost/task/fast_semaphore.hpp>
#include <boost/task/watermark.hpp>

#include <boost/config/abi_prefix.hpp>

namespace boost {
namespace tasks {

template<
	typename Attr,
	typename Comp = std::less< Attr >
>
class bounded_prio_queue
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
	struct compare : public std::binary_function< value_type, value_type, bool >
	{
		bool operator()( value_type const& va1, value_type const& va2)
		{ return Comp()( va1.attr, va2.attr); }
	};

	typedef std::priority_queue<
		value_type,
		std::deque< value_type >,
		compare
	>								queue_type;

	enum state
	{
		ACTIVE = 0,
		DEACTIVE
	};

	atomic< state >			state_;
	queue_type				queue_;
	mutable shared_mutex	mtx_;
	condition				not_full_cond_;
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
					& bounded_prio_queue::producers_activate_,
					this) );
		}
		if ( ! active_() )
			throw task_rejected("queue is not active");
		queue_.push( va);
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
					& bounded_prio_queue::producers_activate_,
					this) ) )
				throw task_rejected("timed out");
		}
		if ( ! active_() )
			throw task_rejected("queue is not active");
		queue_.push( va);
		fsem_.post();
	}

	bool try_take_( callable & ca)
	{
		if ( empty_() )
			return false;
		callable tmp( queue_.top().ca);
		queue_.pop();
		ca.swap( tmp);
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
	bounded_prio_queue(
			fast_semaphore & fsem,
			high_watermark const& hwm,
			low_watermark const& lwm) :
		state_( ACTIVE),
		queue_(),
		mtx_(),
		not_full_cond_(),
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

#endif // BOOST_TASKS_BOUNDED_PRIO_QUEUE_H
