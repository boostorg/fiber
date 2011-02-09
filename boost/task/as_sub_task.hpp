
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TASKS_AS_SUB_TASK_H
#define BOOST_TASKS_AS_SUB_TASK_H

#include <boost/bind.hpp>
#include <boost/config.hpp>
#include <boost/move/move.hpp>

#include <boost/task/callable.hpp>
#include <boost/task/context.hpp>
#include <boost/task/detail/worker.hpp>
#include <boost/task/handle.hpp>
#include <boost/task/new_thread.hpp>
#include <boost/task/spin/future.hpp>
#include <boost/task/task.hpp>

#include <boost/config/abi_prefix.hpp>

namespace boost {
namespace tasks {

struct as_sub_task
{
	template< typename R >
	handle< R > operator()( BOOST_RV_REF( task< R >) t)
	{
		detail::worker * w( detail::worker::tss_get() );
		if ( w)
		{
			spin::promise< R > prom;
			spin::shared_future< R > f( prom.get_future() );
			context ctx;
			handle< R > h( f, ctx);
			w->put( callable( t, boost::move( prom), ctx) );
			return h;
		}
		else
			return new_thread()( t);
	}
};

}}

#include <boost/config/abi_suffix.hpp>

#endif // BOOST_TASKS_AS_SUB_TASK_H
