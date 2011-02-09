
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_THIS_TASKLET_UTILITY_H
#define BOOST_THIS_TASKLET_UTILITY_H

#include <boost/bind.hpp>
#include <boost/config.hpp>
#include <boost/function.hpp>
#include <boost/move/move.hpp>

#include <boost/tasklet/tasklet.hpp>
#include <boost/tasklet/strategy.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace this_tasklet {

inline
bool runs_as_tasklet()
{ return tasklets::strategy::runs_as_tasklet_(); }

inline
tasklet::id get_id()
{ return tasklets::strategy::get_id_(); }

inline
int priority()
{ return tasklets::strategy::priority_(); }

inline
void priority( int prio)
{ tasklets::strategy::priority_( prio); }

inline
void interruption_point()
{ tasklets::strategy::interruption_point_(); }

inline
bool interruption_requested()
{ return tasklets::strategy::interruption_requested_(); }

inline
bool interruption_enabled()
{ return tasklets::strategy::interruption_enabled_(); }

inline
void yield()
{ tasklets::strategy::yield_(); }

inline
void cancel()
{ tasklets::strategy::cancel_(); }

inline
void submit_tasklet( tasklet f)
{ tasklets::strategy::submit_tasklet_( f); }

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_THIS_TASKLET_UTILITY_H
