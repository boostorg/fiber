
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TASKS_ASYNC_H
#define BOOST_TASKS_ASYNC_H

#include <cstddef>

#include <boost/config.hpp>
#include <boost/move/move.hpp>
#include <boost/tasklet.hpp>
#include <boost/thread/future.hpp>

#include <boost/task/as_sub_task.hpp>
#include <boost/task/callable.hpp>
#include <boost/task/context.hpp>
#include <boost/task/handle.hpp>
#include <boost/task/new_thread.hpp>
#include <boost/task/own_thread.hpp>
#include <boost/task/spin/future.hpp>
#include <boost/task/static_pool.hpp>
#include <boost/task/task.hpp>

#include <boost/config/abi_prefix.hpp>

namespace boost {
namespace tasks {

template< typename R >
handle< R > async( task< R > t)
{ return async( boost::move( t) ); }

template< typename R >
handle< R > async( BOOST_RV_REF( task< R >) t)
{ return as_sub_task()( t); }

template< typename R >
handle< R > async( task< R > t, as_sub_task ast)
{ return async( boost::move( t), ast); }

template< typename R >
handle< R > async( BOOST_RV_REF( task< R >) t, as_sub_task ast)
{ return ast( t); }

template< typename R >
handle< R > async( task< R > t, own_thread ot)
{ return async( boost::move( t) ); }

template< typename R >
handle< R > async( BOOST_RV_REF( task< R >) t, own_thread ot)
{ return ot( t); }

template< typename R >
handle< R > async( task< R > t, new_thread nt)
{ return async( boost::move( t), nt); }

template< typename R >
handle< R > async( BOOST_RV_REF( task< R >) t, new_thread nt)
{ return nt( t); }

template< typename R, typename Queue, typename UMS >
handle< R > async( task< R > t, static_pool< Queue, UMS > & pool)
{ return async( boost::move(t), pool); }

template< typename R, typename Queue, typename UMS >
handle< R > async( BOOST_RV_REF( task< R >) t, static_pool< Queue, UMS > & pool)
{ return pool.submit( t); }

template< typename R, typename Attr, typename Queue, typename UMS >
handle< R > async( task< R > t, Attr attr, static_pool< Queue, UMS > & pool)
{ return async( boost::move(t), attr, pool); }

template< typename R, typename Attr, typename Queue, typename UMS >
handle< R > async( BOOST_RV_REF( task< R >) t, Attr attr, static_pool< Queue, UMS > & pool)
{ return pool.submit( t, attr); }

template< typename R, typename Strategy >
handle< R > async(
		task< R > t,
		tasklets::scheduler< Strategy > & sched,
		std::size_t stacksize = tasklet::default_stacksize)
{ return async( boost::move( t), sched, stacksize); }

template< typename R, typename Strategy >
handle< R > async(
		BOOST_RV_REF( task< R >) t,
		tasklets::scheduler< Strategy > & sched,
		std::size_t stacksize = tasklet::default_stacksize)
{
		if ( this_task::runs_in_pool() )
		{
			spin::promise< R > prom;
			spin::shared_future< R > f( prom.get_future() );
			context ctx;
			handle< R > h( f, ctx);
			tasklet fib( callable( t, boost::move( prom), ctx), stacksize);
			sched.submit_tasklet( boost::move( fib) );
			return h;
		}
		else
		{
			promise< R > prom;
			shared_future< R > f( prom.get_future() );
			context ctx;
			handle< R > h( f, ctx);
			tasklet fib(
					callable(
							t,
// TODO: workaround because thread_move_t will be abigous for move
#ifdef BOOST_HAS_RVALUE_REFS
							boost::move( prom),
#else
							boost::detail::thread_move_t< promise< R > >( prom),
#endif
							ctx), stacksize);
			sched.submit_tasklet( boost::move( fib) );
			return h;
		}
}

}}

#include <boost/config/abi_suffix.hpp>

#endif // BOOST_TASKS_ASYNC_H
