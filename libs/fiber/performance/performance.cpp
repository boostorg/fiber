
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include <boost/strand.hpp>
#include <boost/program_options.hpp>

#include "bind_processor.hpp"
#include "performance.hpp"

namespace po = boost::program_options;
namespace pt = boost::posix_time;

volatile int value = 0;

void fn( int i)
{ value = i; }

void test_creation( unsigned int iterations)
{
	int value( 3);
	long total( 0);
	cycle_t overhead( get_overhead() );
	std::cout << "overhead for rdtsc == " << overhead << " cycles" << std::endl;

	boost::fibers::scheduler sched;

	// cache warm-up
	sched.make_strand( fn, value, boost::contexts::stack_helper::default_stacksize());

	for ( unsigned int i = 0; i < iterations; ++i)
	{
		cycle_t start( get_cycles() );
		sched.make_strand( fn, value, boost::contexts::stack_helper::default_stacksize());
		cycle_t diff( get_cycles() - start);
		diff -= overhead;
		BOOST_ASSERT( diff >= 0);
		total += diff;
	}
	std::cout << "average of " << total/iterations << " cycles per creation" << std::endl;
}

void test_switching( unsigned int iterations)
{
	int value( 3);

	boost::fibers::scheduler sched;

	// cache warm-up
	sched.make_strand( fn, value, boost::contexts::stack_helper::default_stacksize());
	for (;;)
	{
		while ( sched.run() );
		if ( sched.empty() ) break;
	}

	for ( unsigned int i = 0; i < iterations; ++i)
		sched.make_strand( fn, value, boost::contexts::stack_helper::default_stacksize());

	cycle_t overhead( get_overhead() );
	std::cout << "overhead for rdtsc == " << overhead << " cycles" << std::endl;

	cycle_t start( get_cycles() );
	for (;;)
	{
		while ( sched.run() );
		if ( sched.empty() ) break;
	}
	cycle_t total( get_cycles() - start);
	total -= overhead * iterations;
	BOOST_ASSERT( total >= 0);
	std::cout << "average of " << total/iterations << " cycles per switch" << std::endl;
}

int main( int argc, char * argv[])
{
	try
	{
		unsigned int iterations( 0);

        po::options_description desc("allowed options");
        desc.add_options()
            ("help,h", "help message")
			("creating,c", "test creation")
			("switching,s", "test switching")
			("iterations,i", po::value< unsigned int >( & iterations), "iterations");

        po::variables_map vm;
        po::store(
            po::parse_command_line(
                argc,
                argv,
                desc),
            vm);
        po::notify( vm);

		if ( vm.count("help") )
		{
			std::cout << desc << std::endl;
			return EXIT_SUCCESS;
		}

		if ( 0 == iterations) throw std::invalid_argument("iterations must be greater than zero");

		bind_to_processor( 0);

		if ( vm.count("creating") ) test_creation( iterations);

		if ( vm.count("switching") ) test_switching( iterations);

		return EXIT_SUCCESS;
	}
	catch ( std::exception const& e)
	{ std::cerr << "exception: " << e.what() << std::endl; }
	catch (...)
	{ std::cerr << "unhandled exception" << std::endl; }
	return EXIT_FAILURE;
}
