
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TASKS_SPIN_COUNT_DOWN_EVENT_H
#define BOOST_TASKS_SPIN_COUNT_DOWN_EVENT_H

#include <cstddef>

#include <boost/atomic.hpp>
#include <boost/thread/thread_time.hpp>
#include <boost/utility.hpp>

#include <boost/task/detail/config.hpp>

namespace boost {
namespace tasks {
namespace spin {

class BOOST_TASKS_DECL count_down_event : private noncopyable
{
private:
	std::size_t				initial_;
	atomic< std::size_t >	current_;

public:
	explicit count_down_event( std::size_t);

	std::size_t initial() const;

	std::size_t current() const;

	bool is_set() const;

	void set();

	void wait();

	bool timed_wait( system_time const&);

	template< typename TimeDuration >
	bool timed_wait( TimeDuration const& rel_time)
	{ return timed_wait( get_system_time() + rel_time); }
};

}}}

#endif // BOOST_TASKS_SPIN_COUNT_DOWN_EVENT_H
