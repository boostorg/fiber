
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TASKLETS_ROUND_ROBIN_H
#define BOOST_TASKLETS_ROUND_ROBIN_H

#include <cstddef>
#include <list>
#include <map>
#include <queue>

#include <boost/config.hpp>
#include <boost/function.hpp>
#include <boost/optional.hpp>
#include <boost/utility.hpp>

#include <boost/tasklet/detail/config.hpp>
#include <boost/tasklet/spin_mutex.hpp>
#include <boost/tasklet/object/id.hpp>
#include <boost/tasklet/tasklet.hpp>
#include <boost/tasklet/strategy.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

# if defined(BOOST_MSVC)
# pragma warning(push)
# pragma warning(disable:4251 4275)
# endif

namespace boost {
namespace tasklets {

class BOOST_TASKLET_DECL round_robin : private noncopyable,
									   public strategy
{
private:
	struct schedulable
	{
		tasklet						f;
		std::list< tasklet::id >	joining_tasklets;
		optional< tasklet::id >		waiting_on_tasklet;
		optional< object::id >		waiting_on_object;

		schedulable() :
			f(), joining_tasklets(),
			waiting_on_tasklet(), waiting_on_object()
		{}

		schedulable( tasklet f_) :
			f( f_), joining_tasklets(),
			waiting_on_tasklet(), waiting_on_object()
		{}

		schedulable( schedulable const& other) :
			f( other.f),
			joining_tasklets( other.joining_tasklets),
			waiting_on_tasklet( other.waiting_on_tasklet),
			waiting_on_object( other.waiting_on_object)
		{}	

		schedulable &
		operator=( schedulable const& other)
		{
			if ( this == & other) return * this;
			f = other.f;
			joining_tasklets = other.joining_tasklets;
			waiting_on_tasklet = other.waiting_on_tasklet;
			waiting_on_object = other.waiting_on_object;
			return * this;
		}
	};

	typedef std::list< tasklet::id >					tasklet_id_list;
	typedef std::map< object::id, tasklet_id_list >	object_map;
	typedef std::map< tasklet::id, schedulable >		tasklet_map;
	typedef std::list< tasklet::id >					runnable_queue;
	typedef std::queue< tasklet::id >					terminated_queue;

	mutable spin_mutex	mtx_;
	tasklet_map					tasklets_;
	object_map					objects_;
	runnable_queue				runnable_tasklets_;
	terminated_queue			terminated_tasklets_;

public:
	round_robin();

	void add( tasklet &);

	void join( tasklet &);

	void interrupt( tasklet &);

	void reschedule( tasklet &);

	void cancel( tasklet &);

	void yield();

	void wait_for_object( object::id const&, spin_mutex::scoped_lock & lk);

	void object_notify_one( object::id const&);

	void object_notify_all( object::id const&);

	void release( tasklet &);

	void migrate( tasklet &);

	void detach_all();

	bool run();

	bool empty() const;

	std::size_t size() const;

	std::size_t ready() const;

	bool has_ready() const;
};

}}

# if defined(BOOST_MSVC)
# pragma warning(pop)
# endif

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_TASKLETS_ROUND_ROBIN_H
