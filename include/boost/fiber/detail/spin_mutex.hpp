
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
//  based on boost::interprocess::sync::interprocess_spin::mutex

#ifndef BOOST_FIBERS_SPIN_MUTEX_H
#define BOOST_FIBERS_SPIN_MUTEX_H

#include <boost/atomic.hpp>
#include <boost/chrono/system_clocks.hpp>
#include <boost/thread/locks.hpp>
#include <boost/utility.hpp>

#include <boost/fiber/detail/config.hpp>

namespace boost {
namespace fibers {
namespace detail {

class BOOST_FIBERS_DECL spin_mutex : private noncopyable
{
private:
	enum state
	{
		LOCKED = 0,
		UNLOCKED
	};

	atomic< state >			state_;

public:
	typedef unique_lock< spin_mutex >	scoped_lock;

	spin_mutex();

	void lock();

	bool try_lock();

	bool timed_lock( chrono::system_clock::time_point const& abs_time);

	template< typename TimeDuration >
	bool timed_lock( TimeDuration const& rel_time)
	{ return timed_lock( chrono::system_clock::now() + rel_time); }

	void unlock();
};

}}}

#endif // BOOST_FIBERS_SPIN_MUTEX_H
