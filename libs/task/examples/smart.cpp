
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <cstdlib>
#include <stdexcept>
#include <string>

#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "boost/task.hpp"

namespace pt = boost::posix_time;
namespace tsk = boost::tasks;

inline
void fibonacci_fn( int n)
{
	if ( n == 0)
	{
		printf("fibonacci(%d) == 0\n", n);
		return;
	}
	if ( n == 1)
	{
		printf("fibonacci(%d) == 1\n", n);
		return;
	}
	int k1( 1), k2( 0);
	for ( int i( 2); i <= n; ++i)
	{
		int tmp( k1);
		k1 = k1 + k2;
		k2 = tmp;
	}
	printf("fibonacci(%d) == %d\n", n, k1);
}

inline
void long_running_fn()
{ boost::this_thread::sleep( pt::milliseconds( 500) ); }

int main( int argc, char *argv[])
{
	try
	{
		tsk::static_pool<
			tsk::unbounded_smart_queue<
					int,
					std::less< int >
			>
		> pool( tsk::poolsize( 1) );

		tsk::task< void > t1( long_running_fn);
		tsk::task< void > t2( fibonacci_fn, 0);
		tsk::task< void > t3( fibonacci_fn, 1);
		tsk::task< void > t4( fibonacci_fn, 10);

		tsk::async(
			boost::move( t1),
			0,
			pool);
		tsk::async(
			boost::move( t2),
			1,
			pool);
		tsk::async(
			boost::move( t3),
			2,
			pool);
		tsk::async(
			boost::move( t4),
			2,
			pool);

		return EXIT_SUCCESS;
	}
	catch ( std::exception const& e)
	{ std::cerr << "exception: " << e.what() << std::endl; }
	catch ( ... )
	{ std::cerr << "unhandled" << std::endl; }

	return EXIT_FAILURE;
}
