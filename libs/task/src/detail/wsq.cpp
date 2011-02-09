
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/task/detail/wsq.hpp"

#include <boost/thread/locks.hpp>

namespace boost {
namespace tasks {
namespace detail {

wsq::wsq( fast_semaphore & fsem) :
	initial_size_( 32),
	array_( new callable[ initial_size_]),
	capacity_( initial_size_),
	mask_( initial_size_ - 1),
	head_idx_( 0),
	tail_idx_( 0),
	mtx_(),
	fsem_( fsem)
{}

bool
wsq::empty() const
{ return head_idx_.load() >= tail_idx_.load(); }

std::size_t
wsq::size() const
{ return tail_idx_.load() - head_idx_.load(); }

void
wsq::put( callable const& ca)
{
	unsigned int tail( tail_idx_.load() );
	if ( tail <= head_idx_.load() + mask_)
	{
		array_[tail & mask_] = ca;
		tail_idx_.fetch_add( 1);
	}
	else
	{
		lock_guard< recursive_mutex > lk( mtx_);
		unsigned int head( head_idx_.load() );
		int count( size() );

		if ( count >= mask_)
		{
			capacity_ <<= 1;
			shared_array< callable > array( new callable[capacity_]);
			for ( int i( 0); i != count; ++i)
				array[i] = array_[(i + head) & mask_];
			array_.swap( array);
			head_idx_.store( 0);
			tail = count;
			tail_idx_.store( tail);
			mask_ = (mask_ << 1) | 1;
		}
		array_[tail & mask_] = ca;
		tail_idx_.fetch_add( 1);
	}
	fsem_.post();
}

bool
wsq::try_take( callable & ca)
{
	unsigned int tail( tail_idx_.load() );
	if ( tail == 0)
		return false;
	tail -= 1;
	tail_idx_.exchange( tail);
	//tail_idx_.store( tail);
	if ( head_idx_.load() <= tail)
	{
		ca.swap( array_[tail & mask_]);
		return true;
	}
	else
	{
		lock_guard< recursive_mutex > lk( mtx_);
		if ( head_idx_.load() <= tail)
		{
			ca.swap( array_[tail & mask_]);
			return true;
		}
		else
		{
			tail_idx_.fetch_add( 1);
			return false;
		}
	}
}

bool
wsq::try_steal( callable & ca)
{
	recursive_mutex::scoped_try_lock lk( mtx_);
	if ( lk.owns_lock() )
	{
		unsigned int head( head_idx_.load() );
		head_idx_.exchange( head + 1);
		//head_idx_.store( head + 1);
		if ( head < tail_idx_.load() )
		{
			ca.swap( array_[head & mask_]);
			return true;
		}
		else
		{
			head_idx_.store( head);
			return false;
		}
	}
	return false;
}

}}}
