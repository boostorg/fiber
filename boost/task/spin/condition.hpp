
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
//  based on boost::interprocess::sync::interprocess_condition

#ifndef BOOST_TASKS_SPIN_CONDITION_H
#define BOOST_TASKS_SPIN_CONDITION_H

#include <cstddef>

#include <boost/assert.hpp>
#include <boost/atomic.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/thread_time.hpp>
#include <boost/utility.hpp>

#include <boost/task/detail/config.hpp>
#include <boost/task/exceptions.hpp>
#include <boost/task/spin/mutex.hpp>
#include <boost/task/utility.hpp>

namespace boost {
namespace tasks {
namespace spin {

class BOOST_TASKS_DECL condition : private noncopyable
{
private:
	enum command
	{
		SLEEPING = 0,
		NOTIFY_ONE,
		NOTIFY_ALL
	};

	atomic< command >		cmd_;
	atomic< std::size_t >	waiters_;
	mutex					enter_mtx_;
	mutex					check_mtx_;

	void notify_( command);

public:
	condition();

	void notify_one();

	void notify_all();

	void wait( unique_lock< mutex > & lk)
	{
		if ( ! lk)
			throw lock_error();
		wait( * lk.mutex() );
	}

	template< typename Pred >
	void wait( unique_lock< mutex > & lk, Pred pred)
	{
		if ( ! lk)
			throw lock_error();

		while ( ! pred() )
			wait( * lk.mutex() );
	}

	bool timed_wait( unique_lock< mutex > & lk, system_time const& abs_time)
	{
		if ( abs_time.is_infinity() )
		{
			wait( lk);
			return true;
		}

		if ( ! lk)
			throw lock_error();
		return timed_wait( * lk.mutex(), abs_time);
	}

	template< typename Pred >
	bool timed_wait( unique_lock< mutex > & lk, system_time const& abs_time, Pred pred)
	{
		if ( abs_time.is_infinity() )
		{
			wait( lk, pred);
			return true;
		}

		if ( ! lk)
			throw lock_error();

		while ( ! pred() )
			if ( ! timed_wait( * lk.mutex(), abs_time) )
				return pred();
		return true;
	}

	template< typename TimeDuration >
	bool timed_wait( unique_lock< mutex > & lk, TimeDuration const& rel_time)
	{ return timed_wait( lk, get_system_time() + rel_time); }

	template<
		typename TimeDuration,
		typename Pred
	>
	bool timed_wait( unique_lock< mutex > & lk, TimeDuration const& rel_time, Pred pred)
	{ return timed_wait( lk, get_system_time() + rel_time, pred); }

	template< typename LockType >
	void wait( LockType & lt)
	{
		{
			mutex::scoped_lock lk( enter_mtx_);
			BOOST_ASSERT( lk);
			waiters_.fetch_add( 1);
			lt.unlock();
		}

		bool unlock_enter_mtx = false;
		for (;;)
		{
			while ( SLEEPING == cmd_.load() )
			{
				this_thread::interruption_point();
				if ( this_task::runs_in_pool() )
					this_task::yield();
				else
					this_thread::yield();	
				this_thread::interruption_point();
			}

			mutex::scoped_lock lk( check_mtx_);
			BOOST_ASSERT( lk);

			command expected = NOTIFY_ONE;
			cmd_.compare_exchange_strong( expected, SLEEPING);
			if ( SLEEPING == expected)
				continue;
			else if ( NOTIFY_ONE == expected)
			{
				unlock_enter_mtx = true;
				waiters_.fetch_sub( 1);
				break;
			}
			else
			{
				unlock_enter_mtx = 1 == waiters_.fetch_sub( 1);
				if ( unlock_enter_mtx)
				{
					expected = NOTIFY_ALL;
					cmd_.compare_exchange_strong( expected, SLEEPING);
				}
				break;
			}
		}

		if ( unlock_enter_mtx)
			enter_mtx_.unlock();

		lt.lock();
	}

	template<
		typename LockType,
		typename Pred
	>
	void wait( LockType & lt, Pred pred)
	{
		while ( ! pred() )
			wait( lt);
	}

	template< typename LockType >
	bool timed_wait( LockType & lt, system_time const& abs_time)
	{
		if ( abs_time.is_infinity() )
		{
			wait( lt);
			return true;
		}

		if ( get_system_time() >= abs_time) return false;

		{
			mutex::scoped_lock lk( enter_mtx_, abs_time);
			BOOST_ASSERT( lk);
			waiters_.fetch_add( 1);
			lt.unlock();
		}

		bool timed_out = false, unlock_enter_mtx = false;
		for (;;)
		{
			while ( SLEEPING == cmd_.load() )
			{
				this_thread::interruption_point();
				if ( this_task::runs_in_pool() )
					this_task::yield();
				else
					this_thread::yield();	
				this_thread::interruption_point();
		
				if ( get_system_time() >= abs_time)
				{
					timed_out = enter_mtx_.try_lock();
					if ( ! timed_out)
						continue;
					break;
				}
			}
		
			if ( timed_out)
			{
				waiters_.fetch_sub( 1);
				unlock_enter_mtx = true;
				break;
			}
			else
			{
				mutex::scoped_lock lk( check_mtx_);
				BOOST_ASSERT( lk);
		
				command expected = NOTIFY_ONE;
				cmd_.compare_exchange_strong( expected, SLEEPING);
				if ( SLEEPING == expected)
					continue;
				else if ( NOTIFY_ONE == expected)
				{
					unlock_enter_mtx = true;
					waiters_.fetch_sub( 1);
					break;
				}
				else
				{
					unlock_enter_mtx = 1 == waiters_.fetch_sub( 1);
					if ( unlock_enter_mtx)
					{
						expected = NOTIFY_ALL;
						cmd_.compare_exchange_strong( expected, SLEEPING);
					}
					break;
				}
			}
		}
		
		if ( unlock_enter_mtx)
			enter_mtx_.unlock();
		
		lt.lock();
		
		return ! timed_out;
	}

	template<
		typename LockType,
		typename Pred
	>
	bool timed_wait( LockType & lt, system_time const& abs_time, Pred pred)
	{
		if ( abs_time.is_infinity() )
		{
			wait( lt, pred);
			return true;
		}

		while ( ! pred() )
			if ( ! wait( lt, abs_time) )
				return pred();
		return true;
	}

	template<
		typename LockType,
		typename TimeDuration
	>
	bool timed_wait( LockType & lt, TimeDuration const& rel_time)
	{ return timed_wait( lt, get_system_time() + rel_time); }

	template<
		typename LockType,
		typename TimeDuration,
		typename Pred
	>
	bool timed_wait( LockType & lt, TimeDuration const& rel_time, Pred pred)
	{ return timed_wait( lt, get_system_time() + rel_time, pred); }
};

}}}

#endif // BOOST_TASKS_SPIN_CONDITION_H
