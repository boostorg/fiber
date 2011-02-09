
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
//  based on boost::interprocess::sync::interprocess_spin_mutex

#ifndef BOOST_TASKLETS_MUTEX_H
#define BOOST_TASKLETS_MUTEX_H

#include <boost/atomic.hpp>
#include <boost/config.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/thread/locks.hpp>
#include <boost/utility.hpp>

#include <boost/tasklet/detail/config.hpp>
#include <boost/tasklet/spin_mutex.hpp>
#include <boost/tasklet/tasklet.hpp>
#include <boost/tasklet/object/id.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

# if defined(BOOST_MSVC)
# pragma warning(push)
# pragma warning(disable:4355 4251 4275)
# endif

namespace boost {
namespace tasklets {

class BOOST_TASKLET_DECL mutex : private noncopyable
{
private:
	enum state
	{
		LOCKED = 0,
		UNLOCKED
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
	state					state_;
	spin_mutex		mtx_;
	container				waiting_tasklets_;
	ordered_idx			&	oidx_;
	sequenced_idx		&	sidx_;

public:
	typedef unique_lock< mutex >	scoped_lock;

	mutex();

	~mutex();

	void lock();

	bool try_lock();

	void unlock();
};

typedef mutex try_mutex;

}}

# if defined(BOOST_MSVC)
# pragma warning(pop)
# endif

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_TASKLETS_MUTEX_H
