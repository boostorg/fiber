
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TASKLETS_AUTO_RESET_EVENT_H
#define BOOST_TASKLETS_AUTO_RESET_EVENT_H

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

class BOOST_TASKLET_DECL auto_reset_event : private noncopyable
{
private:
	enum state
	{
		SET = 0,
		RESET
	};

	atomic< state >		state_;

public:
	explicit auto_reset_event( bool = false);

	void set();

	void wait();

	bool try_wait();
};

}}

# if defined(BOOST_MSVC)
# pragma warning(pop)
# endif

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_TASKLETS_AUTO_RESET_EVENT_H
