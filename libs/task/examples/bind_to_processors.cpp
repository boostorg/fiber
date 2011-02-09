
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <cstdlib>
#include <stdexcept>
#include <vector>

#include <boost/assert.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "boost/task.hpp"

namespace pt = boost::posix_time;
namespace tsk = boost::tasks;

typedef tsk::static_pool< tsk::unbounded_fifo > pool_type;

int serial_fib( int n)
{
	if( n < 2)
		return n;
	else
		return serial_fib( n - 1) + serial_fib( n - 2);
}

int parallel_fib( int n, int cutof)
{
	if ( n < cutof)
	{
		return serial_fib( n);
	}
	else
	{
		BOOST_ASSERT( boost::this_task::runs_in_pool() );
		tsk::handle< int > h1(
			tsk::async(
				tsk::make_task(
					parallel_fib,
					n - 1,
					cutof),
				tsk::as_sub_task() ) );
		tsk::handle< int > h2(
			tsk::async(
				tsk::make_task(
					parallel_fib,
					n - 2,
					cutof),
				tsk::as_sub_task() ) );
		return h1.get() + h2.get();
	}
}

int main( int argc, char *argv[])
{
# if defined(BOOST_HAS_PROCESSOR_BINDINGS)
	try
	{
		pool_type pool( pool_type::bind_to_processors() );

		std::vector< tsk::handle< int > > results;
		results.reserve( 10);

		pt::ptime start = pt::microsec_clock::universal_time();

//	for ( int i = 0; i < 15; ++i)
//		results.push_back(
//			tsk::async(
//				tsk::make_task(
//					parallel_fib,
//					10,
//					5),
//				pool) );
//

		results.push_back(
			tsk::async(
				tsk::make_task(
					parallel_fib,
					25,
					5),
				pool) );

		tsk::waitfor_all( results.begin(), results.end() );

		pt::ptime stop = pt::microsec_clock::universal_time();

		pt::time_duration elapsed = stop - start;
		std::cout << "total microseconds == " << elapsed.total_microseconds() << std::endl;
		std::cout << "total milliseconds == " << elapsed.total_milliseconds() << std::endl;

		int k = 0;
		std::vector< tsk::handle< int > >::iterator e( results.end() );
		for (
			  std::vector< tsk::handle< int > >::iterator i( results.begin() );
			  i != e;
			  ++i)
			std::cout << "fibonacci(" << k++ << ") == " << i->get() << std::endl;

		return EXIT_SUCCESS;
	}
	catch ( std::exception const& e)
	{ std::cerr << "exception: " << e.what() << std::endl; }
	catch ( ... )
	{ std::cerr << "unhandled" << std::endl; }

# endif

	return EXIT_FAILURE;
}


