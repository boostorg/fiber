
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TASKS_UNBOUNDED_PRIO_QUEUE_H
#define BOOST_TASKS_UNBOUNDED_PRIO_QUEUE_H

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

#include <boost/config/abi_prefix.hpp>

namespace boost {
namespace tasks {

template<
	typename Attr,
	typename Comp = std::less< Attr >
>
class unbounded_prio_queue
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
	>						queue_type;

	enum state
	{
		ACTIVE = 0,
		DEACTIVE
	};

	atomic< state >			state_;
	queue_type				queue_;
	mutable shared_mutex	mtx_;
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
		return ! ca.empty();
	}

public:
	unbounded_prio_queue( fast_semaphore & fsem) :
		state_( ACTIVE),
		queue_(),
		mtx_(),
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

#endif // BOOST_TASKS_UNBOUNDED_PRIO_QUEUE_H
