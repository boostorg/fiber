
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TASKS_BOUNDED_FIFO_H
#define BOOST_TASKS_BOUNDED_FIFO_H

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
#include <boost/task/watermark.hpp>

#include <boost/config/abi_prefix.hpp>

namespace boost {
namespace tasks {

class bounded_fifo
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

	atomic< state >			state_;
	atomic< std::size_t >	count_;
	node::sptr_t			head_;
	mutable mutex			head_mtx_;
	node::sptr_t			tail_;
	mutable mutex			tail_mtx_;
	condition				not_full_cond_;
	std::size_t				hwm_;
	std::size_t				lwm_;
	fast_semaphore		&	fsem_;

	bool active_() const
	{ return ACTIVE == state_.load(); }

	void deactivate_()
	{ state_.store( DEACTIVE); }

	std::size_t size_() const
	{ return count_.load(); }

	bool empty_() const
	{ return head_ == get_tail_(); }

	bool full_() const
	{ return size_() >= hwm_; }

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
		count_.fetch_sub( 1);
		return old_head;
	}

public:
	bounded_fifo(
			fast_semaphore & fsem,
			high_watermark const& hwm,
			low_watermark const& lwm) :
		state_( ACTIVE),
		count_( 0),
		head_( new node),
		head_mtx_(),
		tail_( head_),
		tail_mtx_(),
		not_full_cond_(),
		hwm_( hwm),
		lwm_( lwm),
		fsem_( fsem)
	{}

	std::size_t upper_bound() const
	{ return hwm_; }

	std::size_t lower_bound() const
	{ return lwm_; }

	bool active() const
	{ return active_(); }

	void deactivate()
	{
		unique_lock< mutex > lk( head_mtx_);
		deactivate_();
		not_full_cond_.notify_all();
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

			if ( full_() )
			{
				while ( active_() && full_() )
					not_full_cond_.wait( lk);
			}
			if ( ! active_() )
				throw task_rejected("queue is not active");

			tail_->va = va;
			tail_->next = new_node;
			tail_ = new_node;
			count_.fetch_add( 1);
		}
		fsem_.post();
	}

	template< typename TimeDuration >
	void put(
		value_type const& va,
		TimeDuration const& rel_time)
	{
		node::sptr_t new_node( new node);
		{
			unique_lock< mutex > lk( tail_mtx_);

			if ( full_() )
			{
				while ( active_() && full_() )
					if ( ! not_full_cond_.wait( lk, rel_time) )
						throw task_rejected("timed out");
			}
			if ( ! active_() )
				throw task_rejected("queue is not active");

			tail_->va = va;
			tail_->next = new_node;
			tail_ = new_node;
			count_.fetch_add( 1);
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
		bool valid = ! va.empty();
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
};

}}

#include <boost/config/abi_suffix.hpp>

#endif // BOOST_TASKS_BOUNDED_FIFO_H
