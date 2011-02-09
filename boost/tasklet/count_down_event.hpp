
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TASKLETS_COUNT_DOWN_EVENT_H
#define BOOST_TASKLETS_COUNT_DOWN_EVENT_H

#include <cstddef>

#include <boost/atomic.hpp>
#include <boost/config.hpp>
#include <boost/utility.hpp>

#include <boost/tasklet/detail/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

# if defined(BOOST_MSVC)
# pragma warning(push)
# pragma warning(disable:4355 4251 4275)
# endif

namespace boost {
namespace tasklets {

class BOOST_TASKLET_DECL count_down_event : private noncopyable
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
};

}}

# if defined(BOOST_MSVC)
# pragma warning(pop)
# endif

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_TASKLETS_COUNT_DOWN_EVENT_H
