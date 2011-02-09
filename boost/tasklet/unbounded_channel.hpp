
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TASKLETS_UNBOUNDED_CHANNEL_H
#define BOOST_TASKLETS_UNBOUNDED_CHANNEL_H

#include <cstddef>
#include <stdexcept>

#include <boost/atomic.hpp>
#include <boost/config.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/optional.hpp>
#include <boost/utility.hpp>

#include <boost/tasklet/exceptions.hpp>
#include <boost/tasklet/condition.hpp>
#include <boost/tasklet/mutex.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace tasklets {
namespace detail {

template< typename T >
struct unbounded_channel_node
{
	typedef intrusive_ptr< unbounded_channel_node >	ptr;

	atomic< std::size_t >	use_count;
	T						va;
	ptr						next;

	unbounded_channel_node() :
		use_count( 0),
		va(),
		next()
	{}
};

template< typename T >
void intrusive_ptr_add_ref( unbounded_channel_node< T > * p)
{ p->use_count.fetch_add( 1, memory_order_relaxed); }

template< typename T >
void intrusive_ptr_release( unbounded_channel_node< T > * p)
{
	if ( p->use_count.fetch_sub( 1, memory_order_release) == 1)
	{
		atomic_thread_fence( memory_order_acquire);
		delete p;
	}
}

}

template< typename T >
class unbounded_channel : private noncopyable
{
public:
	typedef optional< T >	value_type;

private:
	typedef detail::unbounded_channel_node< value_type >	node_type;

	template< typename X >
	friend void intrusive_ptr_add_ref( unbounded_channel< X > * p);
	template< typename X >
	friend void intrusive_ptr_release( unbounded_channel< X > * p);

	enum state
	{
		ACTIVE = 0,
		DEACTIVE
	};

	atomic< state >				state_;
	typename node_type::ptr		head_;
	mutable mutex				head_mtx_;
	typename node_type::ptr		tail_;
	mutable mutex				tail_mtx_;
	condition					not_empty_cond_;
	atomic< std::size_t >		use_count_;

	bool active_() const
	{ return ACTIVE == state_.load(); }

	void deactivate_()
	{ state_.store( DEACTIVE); }

	bool empty_() const
	{ return head_ == get_tail_(); }

	typename node_type::ptr get_tail_() const
	{
		mutex::scoped_lock lk( tail_mtx_);	
		typename node_type::ptr tmp = tail_;
		return tmp;
	}

	typename node_type::ptr pop_head_()
	{
		typename node_type::ptr old_head = head_;
		head_ = old_head->next;
		return old_head;
	}

public:
	unbounded_channel() :
		state_( ACTIVE),
		head_( new node_type() ),
		head_mtx_(),
		tail_( head_),
		tail_mtx_(),
		not_empty_cond_(),
		use_count_( 0)
	{}

	bool active() const
	{ return active_(); }

	void deactivate()
	{
		mutex::scoped_lock lk( head_mtx_);
		deactivate_();
		not_empty_cond_.notify_all();
	}

	bool empty() const
	{
		mutex::scoped_lock lk( head_mtx_);
		return empty_();
	}

	void put( T const& t)
	{
		typename node_type::ptr new_node( new node_type() );
		{
			mutex::scoped_lock lk( tail_mtx_);

			if ( ! active_() )
				throw std::runtime_error("queue is not active");

			tail_->va = t;
			tail_->next = new_node;
			tail_ = new_node;
		}
		not_empty_cond_.notify_one();
	}

	bool take( value_type & va)
	{
		mutex::scoped_lock lk( head_mtx_);
		bool empty = empty_();
		if ( ! active_() && empty)
			return false;
		if ( empty)
		{
			try
			{
				while ( active_() && empty_() )
					not_empty_cond_.wait( lk);
			}
			catch ( tasklet_interrupted const&)
			{ return false; }
		}
		if ( ! active_() && empty_() )
			return false;
		swap( va, head_->va);
		pop_head_();
		return va;
	}

	bool try_take( value_type & va)
	{
		mutex::scoped_lock lk( head_mtx_);
		if ( empty_() )
			return false;
		swap( va, head_->va);
		pop_head_();
		return va;
	}
};

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_TASKLETS_UNBOUNDED_CHANNEL_H
