
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TASKLETS_SCHEDULER_H
#define BOOST_TASKLETS_SCHEDULER_H

#include <algorithm>
#include <cstddef>

#include <boost/config.hpp>
#include <boost/utility.hpp>

#include <boost/tasklet/exceptions.hpp>
#include <boost/tasklet/tasklet.hpp>
#include <boost/tasklet/round_robin.hpp>
#include <boost/tasklet/strategy.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace tasklets {

class auto_reset_event;
class condition;
class count_down_event;
class manual_reset_event;
class mutex;

template< typename Strategy = round_robin >
class scheduler : private noncopyable
{
private:
	friend class auto_reset_event;
	friend class condition;
	friend class count_down_event;
	friend class manual_reset_event;
	friend class mutex;

	strategy	*	strategy_;

public:
	scheduler() :
		strategy_( new Strategy() )
	{}

	~scheduler()
	{
		strategy_->detach_all();
		delete strategy_;
	} 

	bool run()
	{ return strategy_->run(); }

	bool empty() const
	{ return strategy_->empty(); }

	std::size_t size() const
	{ return strategy_->size(); }

	std::size_t ready() const
	{ return strategy_->ready(); }

	bool has_ready() const
	{ return strategy_->has_ready(); }

	void submit_tasklet( tasklet t)
	{ strategy_->add( t); }

	void migrate_tasklet( tasklet & t)
	{
		if ( ! t) throw tasklet_moved();
		t.impl_->attached_strategy()->release( t);
		strategy_->migrate( t);
	}

	void swap( scheduler & other)
	{ std::swap( strategy_, other.strategy_); }
};

template< typename Strategy >
void swap( scheduler< Strategy > & l, scheduler< Strategy > & r)
{ l.swap( r); }

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_TASKLETS_SCHEDULER_H
