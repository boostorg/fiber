
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <cstdlib>
#include <stdexcept>

#include <boost/assert.hpp>
#include <boost/bind.hpp>
#include <boost/ref.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>

#include "boost/task.hpp"

namespace tsk = boost::tasks;

typedef tsk::static_pool< tsk::unbounded_fifo > pool_type;

void sub_task( int i, int n, tsk::spin::count_down_event & ev)
{
	BOOST_ASSERT( boost::this_task::runs_in_pool() );

	fprintf( stderr, "t%d running ...\n", i);

	ev.set();

	fprintf( stderr, "t%d finished ...\n", i);
}

void main_task(
		pool_type & pool,
		int n,
		tsk::spin::count_down_event & outer_ev)
{
	BOOST_ASSERT( boost::this_task::runs_in_pool() );

	fprintf( stderr, "main-task running %d sub-tasks\n", n);

	tsk::spin::count_down_event inner_ev( n);

	for ( int i = 0; i < n; ++i)
		tsk::async(
				tsk::make_task(
					& sub_task,
					i,
					n,
					boost::ref( inner_ev) ),
				tsk::as_sub_task() );

	inner_ev.wait();
	outer_ev.set();
}

int main( int argc, char *argv[])
{
	try
	{
		tsk::poolsize psize( boost::thread::hardware_concurrency() );
		pool_type pool( psize);

		int n = 32;	
		tsk::spin::count_down_event ev( 1);
		tsk::async(
			tsk::make_task(
				& main_task,
				boost::ref( pool),
				n,
				boost::ref( ev) ),
			pool);

		fprintf( stderr, "main thread: waiting for t0 to finish\n");
		ev.wait();
		fprintf( stderr, "main thread: t0 finished\n");

		return EXIT_SUCCESS;
	}
	catch ( std::exception const& e)
	{ std::cerr << "exception: " << e.what() << std::endl; }
	catch ( ... )
	{ std::cerr << "unhandled" << std::endl; }

	return EXIT_FAILURE;
}
