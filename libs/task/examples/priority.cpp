
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
void print_fn( std::string const& msg)
{ printf("%s", msg.c_str() ); }

inline
void long_running_fn()
{ boost::this_thread::sleep( pt::milliseconds( 250) ); }

int main( int argc, char *argv[])
{
	try
	{
		tsk::static_pool< tsk::unbounded_prio_queue< int > > pool( tsk::poolsize( 1) );

		tsk::task< void > t1( long_running_fn);
		tsk::task< void > t2( print_fn, "a text.\n");
		tsk::task< void > t3( print_fn, " is ");
		tsk::task< void > t4( print_fn, "This");

		tsk::async(
			boost::move( t1),
			3,
			pool);
		tsk::async(
			boost::move( t2),
			0,
			pool);
		tsk::async(
			boost::move( t3),
			1,
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
