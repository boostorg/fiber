
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
//  based on boost::interprocess::sync::interprocess_condition

#ifndef BOOST_TASKLETS_CONDITION_H
#define BOOST_TASKLETS_CONDITION_H

#include <cstddef>

#include <boost/assert.hpp>
#include <boost/atomic.hpp>
#include <boost/config.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/thread.hpp>
#include <boost/utility.hpp>

#include <boost/tasklet/detail/config.hpp>
#include <boost/tasklet/spin_mutex.hpp>
#include <boost/tasklet/exceptions.hpp>
#include <boost/tasklet/mutex.hpp>
#include <boost/tasklet/utility.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

# if defined(BOOST_MSVC)
# pragma warning(push)
# pragma warning(disable:4355 4251 4275)
# endif

namespace boost {
namespace tasklets {

class BOOST_TASKLET_DECL condition : private noncopyable
{
private:
	enum command
	{
		SLEEPING = 0,
		NOTIFY_ONE,
		NOTIFY_ALL
	};

	struct ordered_idx_tag {};
	struct sequenced_idx_tag {};

    typedef boost::multi_index::multi_index_container<
        tasklet,
        boost::multi_index::indexed_by<
            boost::multi_index::ordered_unique<
                boost::multi_index::tag< ordered_idx_tag >,
				boost::multi_index::const_mem_fun<
					tasklet, tasklet::id, & tasklet::get_id
				>
            >,
            boost::multi_index::sequenced<
                boost::multi_index::tag< sequenced_idx_tag >
            >
        >
    >               container;

	typedef container::index< ordered_idx_tag >::type		ordered_idx;
	typedef container::index< sequenced_idx_tag >::type		sequenced_idx;

	object::id				oid_;
	container				waiting_tasklets_;
	ordered_idx			&	oidx_;
	sequenced_idx		&	sidx_;
	atomic< command >		cmd_;
	atomic< std::size_t >	waiters_;
	mutex					enter_mtx_;
	mutex					check_mtx_;
	spin_mutex		mtx_;

public:
	condition();

	~condition();

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
			{
				spin_mutex::scoped_lock lk( mtx_);
				if ( SLEEPING == cmd_.load() )
				{
					if ( this_tasklet::runs_as_tasklet() )
					{
						tasklet * f( strategy::active_tasklet);
						BOOST_ASSERT( f);
						oidx_.insert( * f);
						BOOST_ASSERT( f->impl_->attached_strategy() );
						f->impl_->attached_strategy()->wait_for_object( oid_, lk);
						continue;
					}
					else
					{
						lk.unlock();
						this_thread::yield();
						continue;
					}
				}
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
				BOOST_ASSERT( NOTIFY_ALL == expected);
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
};

}}

# if defined(BOOST_MSVC)
# pragma warning(pop)
# endif

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_TASKLETS_CONDITION_H
