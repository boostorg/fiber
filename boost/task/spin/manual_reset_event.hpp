
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TASKS_SPIN_MANUAL_RESET_EVENT_H
#define BOOST_TASKS_SPIN_MANUAL_RESET_EVENT_H

#include <cstddef>

#include <boost/atomic.hpp>
#include <boost/thread/thread_time.hpp>
#include <boost/utility.hpp>

#include <boost/task/detail/config.hpp>
#include <boost/task/spin/mutex.hpp>

namespace boost {
namespace tasks {
namespace spin {

class BOOST_TASKS_DECL manual_reset_event : private noncopyable
{
private:
	enum state
	{
		SET = 0,
		RESET
	};

	atomic< state >			state_;
	atomic< std::size_t >	waiters_;
	mutex					enter_mtx_;

public:
	explicit manual_reset_event( bool = false);

	void set();

	void reset();

	void wait();

	bool try_wait();

	bool timed_wait( system_time const&);

	template< typename TimeDuration >
	bool timed_wait( TimeDuration const& rel_time)
	{ return timed_wait( get_system_time() + rel_time); }
};

}}}

#endif // BOOST_TASKS_SPIN_MANUAL_RESET_EVENT_H
