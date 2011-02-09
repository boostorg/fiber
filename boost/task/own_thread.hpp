
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TASKS_OWN_THREAD_H
#define BOOST_TASKS_OWN_THREAD_H

#include <boost/config.hpp>
#include <boost/move/move.hpp>
#include <boost/thread/future.hpp>

#include <boost/task/callable.hpp>
#include <boost/task/context.hpp>
#include <boost/task/handle.hpp>
#include <boost/task/spin/future.hpp>
#include <boost/task/task.hpp>
#include <boost/task/utility.hpp>

#include <boost/config/abi_prefix.hpp>

namespace boost {
namespace tasks {

struct own_thread
{
	template< typename R >
	handle< R > operator()( BOOST_RV_REF( task< R >) t)
	{
		if ( this_task::runs_in_pool() )
		{
			spin::promise< R > prom;
			spin::shared_future< R > f( prom.get_future() );
			context ctx;
			handle< R > h( f, ctx);
			callable ca( t, boost::move( prom), ctx);
			ca();
			return h;
		}
		else
		{
			promise< R > prom;
			shared_future< R > f( prom.get_future() );
			context ctx;
			handle< R > h( f, ctx);
			callable ca(
					t,
// TODO: workaround because thread_move_t will be abigous for move
#ifdef BOOST_HAS_RVALUE_REFS
					boost::move( prom),
#else
					boost::detail::thread_move_t< promise< R > >( prom),
#endif
					ctx);
			ca();
			return h;
		}
	}
};

}}

#include <boost/config/abi_suffix.hpp>

#endif // BOOST_TASKS_OWN_THREAD_H
