
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <sstream>
#include <string>

#include <boost/test/unit_test.hpp>
#include <boost/utility.hpp>

#include <boost/tasklet.hpp>

int value1 = 0;
int value2 = 0;
int value3 = 0;
bool interrupted = false;

void fn_1()
{
	for ( int i = 0; i < 5; ++i)
	{
		++value1;
		boost::this_tasklet::yield();
	}
}

void fn_2( boost::tasklet f)
{
	try
	{
		for ( int i = 0; i < 5; ++i)
		{
			++value2;
			if ( i == 1) f.join();
			boost::this_tasklet::yield();
		}
	}
	catch ( boost::tasklets::tasklet_interrupted const&)
	{ interrupted = true; }
}

void fn_3( boost::tasklet f)
{
	for ( int i = 0; i < 5; ++i)
	{
		++value3;
		if ( i == 3) f.cancel();
		boost::this_tasklet::yield();
	}
}

void fn_4( boost::tasklet f)
{
	for ( int i = 0; i < 5; ++i)
	{
		++value3;
		if ( i == 3) f.interrupt();
		boost::this_tasklet::yield();
	}
}

void test_case_1()
{
	value1 = 0;
	value2 = 0;

	boost::tasklets::scheduler<> sched;

	boost::tasklet f( fn_1, boost::tasklet::default_stacksize, boost::protected_stack_allocator());
	sched.submit_tasklet( f);
	sched.submit_tasklet(
		boost::tasklet(
			fn_2, f, boost::tasklet::default_stacksize, boost::protected_stack_allocator()) );

	BOOST_CHECK_EQUAL( 0, value1);
	BOOST_CHECK_EQUAL( 0, value2);

	BOOST_CHECK( sched.run() );
	BOOST_CHECK( ! sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 2), sched.size() );
	BOOST_CHECK_EQUAL( 1, value1);
	BOOST_CHECK_EQUAL( 0, value2);

	BOOST_CHECK( sched.run() );
	BOOST_CHECK( ! sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 2), sched.size() );
	BOOST_CHECK_EQUAL( 1, value1);
	BOOST_CHECK_EQUAL( 1, value2);

	BOOST_CHECK( sched.run() );
	BOOST_CHECK( ! sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 2), sched.size() );
	BOOST_CHECK_EQUAL( 2, value1);
	BOOST_CHECK_EQUAL( 1, value2);

	BOOST_CHECK( sched.run() );
	BOOST_CHECK( ! sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 2), sched.size() );
	BOOST_CHECK_EQUAL( 2, value1);
	BOOST_CHECK_EQUAL( 2, value2);

	BOOST_CHECK( sched.run() );
	BOOST_CHECK( ! sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 2), sched.size() );
	BOOST_CHECK_EQUAL( 3, value1);
	BOOST_CHECK_EQUAL( 2, value2);

	BOOST_CHECK( sched.run() );
	BOOST_CHECK( ! sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 2), sched.size() );
	BOOST_CHECK_EQUAL( 4, value1);
	BOOST_CHECK_EQUAL( 2, value2);

	BOOST_CHECK( sched.run() );
	BOOST_CHECK( ! sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 2), sched.size() );
	BOOST_CHECK_EQUAL( 5, value1);
	BOOST_CHECK_EQUAL( 2, value2);

	BOOST_CHECK( sched.run() );
	BOOST_CHECK( ! sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 1), sched.size() );
	BOOST_CHECK_EQUAL( 5, value1);
	BOOST_CHECK_EQUAL( 2, value2);

	BOOST_CHECK( sched.run() );
	BOOST_CHECK( ! sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 1), sched.size() );
	BOOST_CHECK_EQUAL( 5, value1);
	BOOST_CHECK_EQUAL( 2, value2);

	BOOST_CHECK( sched.run() );
	BOOST_CHECK( ! sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 1), sched.size() );
	BOOST_CHECK_EQUAL( 5, value1);
	BOOST_CHECK_EQUAL( 3, value2);

	BOOST_CHECK( sched.run() );
	BOOST_CHECK( ! sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 1), sched.size() );
	BOOST_CHECK_EQUAL( 5, value1);
	BOOST_CHECK_EQUAL( 4, value2);

	BOOST_CHECK( sched.run() );
	BOOST_CHECK( ! sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 1), sched.size() );
	BOOST_CHECK_EQUAL( 5, value1);
	BOOST_CHECK_EQUAL( 5, value2);

	BOOST_CHECK( sched.run() );
	BOOST_CHECK( sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 0), sched.size() );
	BOOST_CHECK_EQUAL( 5, value1);
	BOOST_CHECK_EQUAL( 5, value2);

	BOOST_CHECK( ! sched.run() );
	BOOST_CHECK( sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 0), sched.size() );
	BOOST_CHECK_EQUAL( 5, value1);
	BOOST_CHECK_EQUAL( 5, value2);
}

void test_case_2()
{
	value1 = 0;
	value2 = 0;
	value3 = 0;

	boost::tasklets::scheduler<> sched;

	boost::tasklet f( fn_1, boost::tasklet::default_stacksize, boost::protected_stack_allocator());
	sched.submit_tasklet( f);
	sched.submit_tasklet(
		boost::tasklet(
			fn_2, f, boost::tasklet::default_stacksize, boost::protected_stack_allocator()) );
	sched.submit_tasklet(
		boost::tasklet(
			fn_3, f, boost::tasklet::default_stacksize, boost::protected_stack_allocator()) );

	BOOST_CHECK_EQUAL( 0, value1);
	BOOST_CHECK_EQUAL( 0, value2);
	BOOST_CHECK_EQUAL( 0, value3);

	BOOST_CHECK( sched.run() );
	BOOST_CHECK( ! sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 3), sched.size() );
	BOOST_CHECK_EQUAL( 1, value1);
	BOOST_CHECK_EQUAL( 0, value2);
	BOOST_CHECK_EQUAL( 0, value3);

	BOOST_CHECK( sched.run() );
	BOOST_CHECK( ! sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 3), sched.size() );
	BOOST_CHECK_EQUAL( 1, value1);
	BOOST_CHECK_EQUAL( 1, value2);
	BOOST_CHECK_EQUAL( 0, value3);

	BOOST_CHECK( sched.run() );
	BOOST_CHECK( ! sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 3), sched.size() );
	BOOST_CHECK_EQUAL( 1, value1);
	BOOST_CHECK_EQUAL( 1, value2);
	BOOST_CHECK_EQUAL( 1, value3);

	BOOST_CHECK( sched.run() );
	BOOST_CHECK( ! sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 3), sched.size() );
	BOOST_CHECK_EQUAL( 2, value1);
	BOOST_CHECK_EQUAL( 1, value2);
	BOOST_CHECK_EQUAL( 1, value3);

	BOOST_CHECK( sched.run() );
	BOOST_CHECK( ! sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 3), sched.size() );
	BOOST_CHECK_EQUAL( 2, value1);
	BOOST_CHECK_EQUAL( 2, value2);
	BOOST_CHECK_EQUAL( 1, value3);

	BOOST_CHECK( sched.run() );
	BOOST_CHECK( ! sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 3), sched.size() );
	BOOST_CHECK_EQUAL( 2, value1);
	BOOST_CHECK_EQUAL( 2, value2);
	BOOST_CHECK_EQUAL( 2, value3);

	BOOST_CHECK( sched.run() );
	BOOST_CHECK( ! sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 3), sched.size() );
	BOOST_CHECK_EQUAL( 3, value1);
	BOOST_CHECK_EQUAL( 2, value2);
	BOOST_CHECK_EQUAL( 2, value3);

	BOOST_CHECK( sched.run() );
	BOOST_CHECK( ! sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 3), sched.size() );
	BOOST_CHECK_EQUAL( 3, value1);
	BOOST_CHECK_EQUAL( 2, value2);
	BOOST_CHECK_EQUAL( 3, value3);

	BOOST_CHECK( sched.run() );
	BOOST_CHECK( ! sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 3), sched.size() );
	BOOST_CHECK_EQUAL( 4, value1);
	BOOST_CHECK_EQUAL( 2, value2);
	BOOST_CHECK_EQUAL( 3, value3);

	BOOST_CHECK( sched.run() );
	BOOST_CHECK( ! sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 2), sched.size() );
	BOOST_CHECK_EQUAL( 4, value1);
	BOOST_CHECK_EQUAL( 2, value2);
	BOOST_CHECK_EQUAL( 4, value3);

	BOOST_CHECK( sched.run() );
	BOOST_CHECK( ! sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 2), sched.size() );
	BOOST_CHECK_EQUAL( 4, value1);
	BOOST_CHECK_EQUAL( 2, value2);
	BOOST_CHECK_EQUAL( 4, value3);

	BOOST_CHECK( sched.run() );
	BOOST_CHECK( ! sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 2), sched.size() );
	BOOST_CHECK_EQUAL( 4, value1);
	BOOST_CHECK_EQUAL( 2, value2);
	BOOST_CHECK_EQUAL( 5, value3);

	BOOST_CHECK( sched.run() );
	BOOST_CHECK( ! sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 2), sched.size() );
	BOOST_CHECK_EQUAL( 4, value1);
	BOOST_CHECK_EQUAL( 3, value2);
	BOOST_CHECK_EQUAL( 5, value3);

	BOOST_CHECK( sched.run() );
	BOOST_CHECK( ! sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 1), sched.size() );
	BOOST_CHECK_EQUAL( 4, value1);
	BOOST_CHECK_EQUAL( 3, value2);
	BOOST_CHECK_EQUAL( 5, value3);

	BOOST_CHECK( sched.run() );
	BOOST_CHECK( ! sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 1), sched.size() );
	BOOST_CHECK_EQUAL( 4, value1);
	BOOST_CHECK_EQUAL( 4, value2);
	BOOST_CHECK_EQUAL( 5, value3);

	BOOST_CHECK( sched.run() );
	BOOST_CHECK( ! sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 1), sched.size() );
	BOOST_CHECK_EQUAL( 4, value1);
	BOOST_CHECK_EQUAL( 5, value2);
	BOOST_CHECK_EQUAL( 5, value3);

	BOOST_CHECK( sched.run() );
	BOOST_CHECK( sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 0), sched.size() );
	BOOST_CHECK_EQUAL( 4, value1);
	BOOST_CHECK_EQUAL( 5, value2);
	BOOST_CHECK_EQUAL( 5, value3);

	BOOST_CHECK( ! sched.run() );
	BOOST_CHECK( sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 0), sched.size() );
	BOOST_CHECK_EQUAL( 4, value1);
	BOOST_CHECK_EQUAL( 5, value2);
	BOOST_CHECK_EQUAL( 5, value3);
}

void test_case_3()
{
	value1 = 0;
	value2 = 0;
	value3 = 0;

	boost::tasklets::scheduler<> sched;

	boost::tasklet f1( fn_1, boost::tasklet::default_stacksize, boost::protected_stack_allocator());
	sched.submit_tasklet( f1);
	boost::tasklet f2( fn_2, f1, boost::tasklet::default_stacksize, boost::protected_stack_allocator());
	sched.submit_tasklet( f2);
	sched.submit_tasklet(
		boost::tasklet(
			fn_4, f2, boost::tasklet::default_stacksize, boost::protected_stack_allocator()) );

	BOOST_CHECK( ! sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 3), sched.size() );

	BOOST_CHECK_EQUAL( 0, value1);
	BOOST_CHECK_EQUAL( 0, value2);
	BOOST_CHECK_EQUAL( 0, value3);
	BOOST_CHECK_EQUAL( false, interrupted);

	for (;;)
	{
		while ( sched.run() );
		if ( sched.empty() ) break;
	}

	BOOST_CHECK( ! sched.run() );
	BOOST_CHECK( sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 0), sched.size() );
	BOOST_CHECK_EQUAL( 5, value1);
	BOOST_CHECK_EQUAL( 2, value2);
	BOOST_CHECK_EQUAL( 5, value3);
	BOOST_CHECK_EQUAL( true, interrupted);
}

boost::unit_test::test_suite * init_unit_test_suite( int, char* [])
{
	boost::unit_test::test_suite * test =
		BOOST_TEST_SUITE("Boost.Tasklet: join test suite");

	test->add( BOOST_TEST_CASE( & test_case_1) );
	test->add( BOOST_TEST_CASE( & test_case_2) );
	test->add( BOOST_TEST_CASE( & test_case_3) );

	return test;
}
