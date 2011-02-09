
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TASKLETS_STRATEGY_H
#define BOOST_TASKLETS_STRATEGY_H

#include <cstddef>

#include <boost/config.hpp>
#include <boost/move/move.hpp>

#include <boost/tasklet/detail/interrupt_flags.hpp>
#include <boost/tasklet/spin_mutex.hpp>
#include <boost/tasklet/object/id.hpp>
#include <boost/tasklet/tasklet.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

# if defined(BOOST_MSVC)
# pragma warning(push)
# pragma warning(disable:4251)
# endif

namespace boost {

namespace this_tasklet {

bool runs_as_tasklet();
tasklet::id get_id();
void yield();
void cancel();
int priority();
void priority( int);
void interruption_point();
bool interruption_requested();
bool interruption_enabled();
void submit_tasklet( tasklet);

class disable_interruption;
class restore_interruption;

}

namespace tasklets {

class auto_reset_event;
class condition;
class count_down_event;
class manual_reset_event;
class mutex;

class BOOST_TASKLET_DECL strategy
{
private:
	friend bool this_tasklet::runs_as_tasklet();
	friend tasklet::id this_tasklet::get_id();
	friend void this_tasklet::yield();
	friend void this_tasklet::cancel();
	friend int this_tasklet::priority();
	friend void this_tasklet::priority( int);
	friend void this_tasklet::interruption_point();
	friend bool this_tasklet::interruption_requested();
	friend bool this_tasklet::interruption_enabled();
	friend void this_tasklet::submit_tasklet( tasklet);
	friend class this_tasklet::disable_interruption;
	friend class this_tasklet::restore_interruption;
	friend class auto_reset_event;
	friend class condition;
	friend class count_down_event;
	friend class manual_reset_event;
	friend class mutex;

	static bool runs_as_tasklet_();

	static tasklet::id get_id_();

	static void interruption_point_();

	static bool interruption_requested_();

	static detail::interrupt_type & interrupt_flags_();

	static bool interruption_enabled_();

	static int priority_();

	static void priority_( int);

	static void yield_();

	static void cancel_();

	static void submit_tasklet_( tasklet);

protected:
	static BOOST_TASKLET_TSSDECL tasklet	*	active_tasklet;

	static void call( tasklet &);

	static void yield( tasklet &);

	static void detach( tasklet &);

	static void enable_interruption( tasklet &);

	static bool interruption_enabled( tasklet const&);

	static bool in_state_not_started( tasklet const&);

	static bool in_state_ready( tasklet const&);

	static bool in_state_running( tasklet const&);

	static bool in_state_wait_for_tasklet( tasklet const&);

	static bool in_state_wait_for_object( tasklet const&);

	static bool in_state_terminated( tasklet const&);

	static void set_state_ready( tasklet &);

	static void set_state_running( tasklet &);

	static void set_state_wait_for_tasklet( tasklet &);

	static void set_state_wait_for_object( tasklet &);

	static void set_state_terminated( tasklet &);

	void attach( tasklet &);

public:
	strategy();

	virtual ~strategy();

	virtual void add( tasklet &) = 0;

	virtual void join( tasklet &) = 0;

	virtual void interrupt( tasklet &) = 0;

	virtual void reschedule( tasklet &) = 0;

	virtual void cancel( tasklet &) = 0;
	
	virtual void yield() = 0;

	virtual void wait_for_object( object::id const&, spin_mutex::scoped_lock & lk) = 0;

	virtual void object_notify_one( object::id const&) = 0;

	virtual void object_notify_all( object::id const&) = 0;

	virtual void release( tasklet &) = 0;

	virtual void migrate( tasklet &) = 0;

	virtual void detach_all() = 0;

	virtual bool run() = 0;

	virtual bool empty() const = 0;

	virtual std::size_t size() const = 0;

	virtual std::size_t ready() const = 0;

	virtual bool has_ready() const = 0;
};

}}

# if defined(BOOST_MSVC)
# pragma warning(pop)
# endif

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_TASKLETS_STRATEGY_H
