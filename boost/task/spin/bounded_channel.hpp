
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TASKS_SPIN_BOUNDED_CHANNEL_H
#define BOOST_TASKS_SPIN_BOUNDED_CHANNEL_H

#include <cstddef>
#include <stdexcept>

#include <boost/atomic.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/optional.hpp>
#include <boost/thread/thread_time.hpp>
#include <boost/utility.hpp>

#include <boost/task/exceptions.hpp>
#include <boost/task/spin/condition.hpp>
#include <boost/task/spin/mutex.hpp>

#include <boost/config/abi_prefix.hpp>

namespace boost {
namespace tasks {
namespace spin {
namespace detail {

template< typename T >
struct bounded_channel_base_node
{
	typedef intrusive_ptr< bounded_channel_base_node >	ptr;

	atomic< std::size_t >	use_count;
	T						va;
	ptr						next;

	bounded_channel_base_node() :
		use_count( 0),
		va(),
		next()
	{}
};

template< typename T >
void intrusive_ptr_add_ref( bounded_channel_base_node< T > * p)
{ p->use_count.fetch_add( 1, memory_order_relaxed); }

template< typename T >
void intrusive_ptr_release( bounded_channel_base_node< T > * p)
{
	if ( p->use_count.fetch_sub( 1, memory_order_release) == 1)
	{
		atomic_thread_fence( memory_order_acquire);
		delete p;
	}
}

template< typename T >
class bounded_channel_base : private noncopyable
{
public:
	typedef optional< T >	value_type;

private:
	typedef bounded_channel_base_node< value_type >		node_type;

	template< typename X >
	friend void intrusive_ptr_add_ref( bounded_channel_base< X > * p);
	template< typename X >
	friend void intrusive_ptr_release( bounded_channel_base< X > * p);

	enum state
	{
		ACTIVE = 0,
		DEACTIVE
	};

	atomic< state >				state_;
	atomic< std::size_t >		count_;
	typename node_type::ptr		head_;
	mutable mutex				head_mtx_;
	typename node_type::ptr		tail_;
	mutable mutex				tail_mtx_;
	condition					not_empty_cond_;
	condition					not_full_cond_;
	unsigned int				hwm_;
	unsigned int				lwm_;
	atomic< std::size_t >		use_count_;

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
		count_.fetch_sub( 1);
		return old_head;
	}

public:
	bounded_channel_base(
			std::size_t hwm,
			std::size_t lwm) :
		state_( ACTIVE),
		count_( 0),
		head_( new node_type() ),
		head_mtx_(),
		tail_( head_),
		tail_mtx_(),
		not_empty_cond_(),
		not_full_cond_(),
		hwm_( hwm),
		lwm_( lwm),
		use_count_( 0)
	{
		if ( hwm_ < lwm_)
			throw invalid_watermark();
	}

	bounded_channel_base( std::size_t wm) :
		state_( ACTIVE),
		count_( 0),
		head_( new node_type() ),
		head_mtx_(),
		tail_( head_),
		tail_mtx_(),
		not_empty_cond_(),
		not_full_cond_(),
		hwm_( wm),
		lwm_( wm),
		use_count_( 0)
	{}

	std::size_t upper_bound() const
	{ return hwm_; }

	std::size_t lower_bound() const
	{ return lwm_; }

	bool active() const
	{ return active_(); }

	void deactivate()
	{
		mutex::scoped_lock head_lk( head_mtx_);
		mutex::scoped_lock tail_lk( tail_mtx_);
		not_empty_cond_.notify_all();
		not_full_cond_.notify_all();
		deactivate_();
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

			if ( full_() )
			{
				while ( active_() && full_() )
					not_full_cond_.wait( lk);
			}

			if ( ! active_() )
				throw std::runtime_error("queue is not active");

			tail_->va = t;
			tail_->next = new_node;
			tail_ = new_node;
			count_.fetch_add( 1);
		}
		not_empty_cond_.notify_one();
	}

	bool put( T const& t, system_time const& abs_time)
	{
		typename node_type::ptr new_node( new node_type() );
		{
			mutex::scoped_lock lk( tail_mtx_);

			if ( full_() )
			{
				while ( active_() && full_() )
					if ( ! not_full_cond_.timed_wait( lk, abs_time) );
						return false;
			}

			if ( ! active_() )
				throw std::runtime_error("queue is not active");

			tail_->va = t;
			tail_->next = new_node;
			tail_ = new_node;
			count_.fetch_add( 1);
		}
		not_empty_cond_.notify_one();
		return true;
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
			catch ( task_interrupted const&)
			{ return false; }
		}
		if ( ! active_() && empty_() )
			return false;
		swap( va, head_->va);
		pop_head_();
		if ( size_() <= lwm_)
		{
			if ( lwm_ == hwm_)
				not_full_cond_.notify_one();
			else
				// more than one producer could be waiting
				// for submiting an action object
				not_full_cond_.notify_all();
		}
		return va;
	}

	bool take( value_type & va, system_time const& abs_time)
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
					if ( ! not_empty_cond_.timed_wait( lk, abs_time) )
						return false;
			}
			catch ( task_interrupted const&)
			{ return false; }
		}
		if ( ! active_() && empty_() )
			return false;
		swap( va, head_->va);
		pop_head_();
		if ( size_() <= lwm_)
		{
			if ( lwm_ == hwm_)
				not_full_cond_.notify_one();
			else
				// more than one producer could be waiting
				// for submiting an action object
				not_full_cond_.notify_all();
		}
		return va;
	}

	bool try_take( value_type & va)
	{
		mutex::scoped_lock lk( head_mtx_);
		if ( empty_() )
			return false;
		swap( va, head_->va);
		pop_head_();
		bool valid = va;
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

template< typename T >
void intrusive_ptr_add_ref( bounded_channel_base< T > * p)
{ p->use_count_.fetch_add( 1, memory_order_relaxed); }

template< typename T >
void intrusive_ptr_release( bounded_channel_base< T > * p)
{
	if ( p->use_count_.fetch_sub( 1, memory_order_release) == 1)
	{
		atomic_thread_fence( memory_order_acquire);
		delete p;
	}
}

}

template< typename T >
class bounded_channel
{
private:
	typedef typename detail::bounded_channel_base< T >::value_type	value_type;

	intrusive_ptr< detail::bounded_channel_base< T > >	base_;

public:
	bounded_channel(
			std::size_t hwm,
			std::size_t lwm) :
		base_( new detail::bounded_channel_base< T >( hwm, lwm) )
	{}
	
	bounded_channel( std::size_t wm) :
		base_( new detail::bounded_channel_base< T >( wm) )
	{}
	
	void upper_bound( std::size_t hwm)
	{ base_->upper_bound( hwm); }
	
	std::size_t upper_bound() const
	{ return base_->upper_bound(); }
	
	void lower_bound( std::size_t lwm)
	{ base_->lower_bound( lwm); }
	
	std::size_t lower_bound() const
	{ return base_->lower_bound(); }

	bool active() const
	{ return base_->active(); }
	
	void deactivate()
	{ base_->deactivate(); }
	
	bool empty() const
	{ return base_->empty(); }
	
	void put( T const& t)
	{ base_->put( t); }
	
	bool put( T const& t, system_time const& abs_time)
	{ return base_->put( t, abs_time); }

	template< typename TimeDuration >
	bool put( T const& t, TimeDuration const& rel_time)
	{ return base_->put( t, get_system_time() + rel_time); }

	bool take( value_type & va)
	{ return base_->take( va);}

	bool take( value_type & va, system_time const& abs_time)
	{ return base_->take( va, abs_time);}

	template< typename TimeDuration >
	bool take( value_type & va, TimeDuration const& rel_time)
	{ return base_->take( va, get_system_time() + rel_time);}
	
	bool try_take( value_type & va)
	{ return base_->try_take( va); }
};

}}}

#include <boost/config/abi_suffix.hpp>

#endif // BOOST_TASKS_SPIN_BOUNDED_CHANNEL_H
