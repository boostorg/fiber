
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_COUNT_DOWN_EVENT_H
#define BOOST_FIBERS_COUNT_DOWN_EVENT_H

#include <cstddef>
#include <deque>

#include <boost/chrono/system_clocks.hpp>
#include <boost/config.hpp>
#include <boost/utility.hpp>

#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/fiber_base.hpp>
#include <boost/fiber/detail/spin_mutex.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

# if defined(BOOST_MSVC)
# pragma warning(push)
# pragma warning(disable:4355 4251 4275)
# endif

namespace boost {
namespace fibers {

class BOOST_FIBERS_DECL count_down_event : private noncopyable
{
private:
	std::size_t			                    initial_;
	atomic< std::size_t >                   current_;
    detail::spin_mutex                      waiting_mtx_;
    std::deque< detail::fiber_base::ptr_t > waiting_;

public:
	explicit count_down_event( std::size_t);

	std::size_t initial() const;

	std::size_t current() const;

	void wait();

    template< typename TimeDuration >
	bool timed_wait( TimeDuration const& dt)
    { return timed_wait( chrono::system_clock::now() + dt); }

    bool timed_wait( chrono::system_clock::time_point const& abs_time);

	bool is_set() const;

	void set();
};

}}

# if defined(BOOST_MSVC)
# pragma warning(pop)
# endif

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_COUNT_DOWN_EVENT_H
