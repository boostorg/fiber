
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)txt)

#ifndef BOOST_TASKS_UTILITY_H
#define BOOST_TASKS_UTILITY_H

#include <boost/assert.hpp>
#include <boost/thread.hpp>

#include <boost/task/detail/worker.hpp>

#include <boost/config/abi_prefix.hpp>

namespace boost {
namespace this_task {

inline
void yield()
{
	tasks::detail::worker * w( tasks::detail::worker::tss_get() );
	BOOST_ASSERT( w);
	w->yield();
}

inline
bool runs_in_pool()
{ return tasks::detail::worker::tss_get() != 0; }

inline
thread::id worker_id()
{
	tasks::detail::worker * w( tasks::detail::worker::tss_get() );
	BOOST_ASSERT( w);
	return w->get_id();
}

}}

#include <boost/config/abi_suffix.hpp>

#endif // BOOST_TASKS_UTILITY_H
