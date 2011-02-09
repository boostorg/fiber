
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
//  based on boost::interprocess::sync::interprocess_spin_mutex

#ifndef BOOST_TASKLETS_SPIN_MUTEX_H
#define BOOST_TASKLETS_SPIN_MUTEX_H

#include <boost/atomic.hpp>
#include <boost/config.hpp>
#include <boost/thread/locks.hpp>
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

class BOOST_TASKLET_DECL spin_mutex : private noncopyable
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

	void unlock();
};

typedef spin_mutex try_spin_mutex;

}}

# if defined(BOOST_MSVC)
# pragma warning(pop)
# endif

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_TASKLETS_SPIN_MUTEX_H
