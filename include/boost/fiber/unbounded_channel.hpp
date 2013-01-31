
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
//  idea of node-base locking from 'C++ Concurrency in Action', Anthony Williams

#ifndef BOOST_FIBERS_UNBOUNDED_CHANNEL_H
#define BOOST_FIBERS_UNBOUNDED_CHANNEL_H

#include <cstddef>
#include <stdexcept>

#include <boost/chrono/system_clocks.hpp>
#include <boost/config.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/optional.hpp>
#include <boost/utility.hpp>

#include <boost/fiber/exceptions.hpp>
#include <boost/fiber/condition.hpp>
#include <boost/fiber/mutex.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace detail {

template< typename T >
struct unbounded_channel_node
{
	typedef intrusive_ptr< unbounded_channel_node >	ptr;

	std::size_t 	use_count;
	T				va;
	ptr				next;

	unbounded_channel_node() :
		use_count( 0),
		va(),
		next()
	{}
};

template< typename T >
void intrusive_ptr_add_ref( unbounded_channel_node< T > * p)
{ ++p->use_count; }

template< typename T >
void intrusive_ptr_release( unbounded_channel_node< T > * p)
{ if ( 0 == --p->use_count) delete p; }

}

template< typename T >
class unbounded_channel : private noncopyable
{
public:
	typedef optional< T >	value_type;

private:
	typedef detail::unbounded_channel_node< value_type >	node_type;

	enum state
	{
		ACTIVE = 0,
		DEACTIVE
	};

	state       				state_;
	typename node_type::ptr		head_;
	mutable mutex				head_mtx_;
	typename node_type::ptr		tail_;
	mutable mutex				tail_mtx_;
	condition					not_empty_cond_;

	bool active_() const
	{ return ACTIVE == state_; }

	void deactivate_()
	{ state_ = DEACTIVE; }

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
		not_empty_cond_()
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
//  		try
//  		{
				while ( active_() && empty_() )
					not_empty_cond_.wait( lk);
//  		}
//  		catch ( fibers_interrupted const&)
//  		{ return false; }
		}
		if ( ! active_() && empty_() )
			return false;
		swap( va, head_->va);
		pop_head_();
		return va;
	}
#if 0
    template< typename TimeDuration >
	bool take( value_type & va, TimeDuration const& dt)
	{ return take( va, chrono::system_clock::now() + dt); }

	bool take( value_type & va, chrono::system_clock::time_point const& abs_time)
	{
		mutex::scoped_lock lk( head_mtx_);
		bool empty = empty_();
		if ( ! active_() && empty)
			return false;
		if ( empty)
		{
//  		try
//  		{
				while ( active_() && empty_() )
                {
					if ( ! not_empty_cond_.timed_wait( lk, abs_time) )
                        return false;
                }
//  		}
//  		catch ( fibers_interrupted const&)
//  		{ return false; }
		}
		if ( ! active_() && empty_() )
			return false;
		swap( va, head_->va);
		pop_head_();
		return va;
	}
#endif
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

#endif // BOOST_FIBERS_UNBOUNDED_CHANNEL_H
