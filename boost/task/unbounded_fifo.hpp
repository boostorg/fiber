
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TASKS_UNBOUNDED_FIFO_H
#define BOOST_TASKS_UNBOUNDED_FIFO_H

#include <cstddef>

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

class unbounded_fifo
{
public:
	typedef detail::has_no_attribute	attribute_tag_type;
	typedef callable					value_type;

private:
	struct node
	{
		typedef shared_ptr< node >	sptr_t;

		value_type	va;
		sptr_t		next;
	};

	enum state
	{
		ACTIVE = 0,
		DEACTIVE
	};

	atomic< state >		state_;
	node::sptr_t		head_;
	mutable mutex		head_mtx_;
	node::sptr_t		tail_;
	mutable mutex		tail_mtx_;
	fast_semaphore	&	fsem_;

	bool active_() const
	{ return ACTIVE == state_.load(); }

	void deactivate_()
	{ state_.store( DEACTIVE); }

	bool empty_() const
	{ return head_ == get_tail_(); }

	node::sptr_t get_tail_() const
	{
		lock_guard< mutex > lk( tail_mtx_);	
		node::sptr_t tmp = tail_;
		return tmp;
	}

	node::sptr_t pop_head_()
	{
		node::sptr_t old_head = head_;
		head_ = old_head->next;
		return old_head;
	}

public:
	unbounded_fifo( fast_semaphore & fsem) :
		state_( ACTIVE),
		head_( new node),
		head_mtx_(),
		tail_( head_),
		tail_mtx_(),
		fsem_( fsem)
	{}

	bool active() const
	{ return active_(); }

	void deactivate()
	{
		unique_lock< mutex > lk( head_mtx_);
		deactivate_();
	}

	bool empty() const
	{
		unique_lock< mutex > lk( head_mtx_);
		return empty_();
	}

	void put( value_type const& va)
	{
		node::sptr_t new_node( new node);
		{
			unique_lock< mutex > lk( tail_mtx_);
			if ( ! active_() )
				throw task_rejected("queue is not active");
			tail_->va = va;
			tail_->next = new_node;
			tail_ = new_node;
		}
		fsem_.post();
	}

	bool try_take( value_type & va)
	{
		unique_lock< mutex > lk( head_mtx_);
		if ( empty_() )
			return false;
		va.swap( head_->va);
		pop_head_();
		return ! va.empty();
	}
};

}}

#include <boost/config/abi_suffix.hpp>

#endif // BOOST_TASKS_UNBOUNDED_FIFO_H
