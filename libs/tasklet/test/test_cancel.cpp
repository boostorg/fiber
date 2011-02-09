
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

void fn_1()
{
	for ( int i = 0; i < 3; ++i)
	{
		++value1;
		boost::this_tasklet::yield();
		if ( i == 1)
			boost::this_tasklet::cancel();
	}
}

void fn_2()
{
	for ( int i = 0; i < 3; ++i)
	{
		++value2;
		boost::this_tasklet::yield();
	}
}

void fn_3( boost::tasklet f)
{
	for ( int i = 0; i < 3; ++i)
	{
		++value3;
		if ( i == 1) f.cancel();
		boost::this_tasklet::yield();
	}
}

void test_case_1()
{
	value1 = 0;
	value2 = 0;

	boost::tasklets::scheduler<> sched;

	sched.submit_tasklet(
		boost::tasklet(
			fn_1, boost::tasklet::default_stacksize, boost::protected_stack_allocator()) );
	sched.submit_tasklet(
		boost::tasklet(
			fn_2, boost::tasklet::default_stacksize, boost::protected_stack_allocator()) );

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
	BOOST_CHECK_EQUAL( std::size_t( 1), sched.size() );
	BOOST_CHECK_EQUAL( 2, value1);
	BOOST_CHECK_EQUAL( 2, value2);

	BOOST_CHECK( sched.run() );
	BOOST_CHECK( ! sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 1), sched.size() );
	BOOST_CHECK_EQUAL( 2, value1);
	BOOST_CHECK_EQUAL( 3, value2);

	BOOST_CHECK( sched.run() );
	BOOST_CHECK( sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 0), sched.size() );
	BOOST_CHECK_EQUAL( 2, value1);
	BOOST_CHECK_EQUAL( 3, value2);

	BOOST_CHECK( ! sched.run() );
	BOOST_CHECK( sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 0), sched.size() );
	BOOST_CHECK_EQUAL( 2, value1);
	BOOST_CHECK_EQUAL( 3, value2);
}

void test_case_2()
{
	value2 = 0;
	value3 = 0;

	boost::tasklets::scheduler<> sched;

	boost::tasklet f( fn_2, boost::tasklet::default_stacksize, boost::protected_stack_allocator());
	sched.submit_tasklet( f);
	sched.submit_tasklet(
		boost::tasklet(
			fn_3, f, boost::tasklet::default_stacksize, boost::protected_stack_allocator()) );

	BOOST_CHECK_EQUAL( 0, value2);
	BOOST_CHECK_EQUAL( 0, value3);

	BOOST_CHECK( sched.run() );
	BOOST_CHECK( ! sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 2), sched.size() );
	BOOST_CHECK_EQUAL( 1, value2);
	BOOST_CHECK_EQUAL( 0, value3);

	BOOST_CHECK( sched.run() );
	BOOST_CHECK( ! sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 2), sched.size() );
	BOOST_CHECK_EQUAL( 1, value2);
	BOOST_CHECK_EQUAL( 1, value3);

	BOOST_CHECK( sched.run() );
	BOOST_CHECK( ! sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 2), sched.size() );
	BOOST_CHECK_EQUAL( 2, value2);
	BOOST_CHECK_EQUAL( 1, value3);

	BOOST_CHECK( sched.run() );
	BOOST_CHECK( ! sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 1), sched.size() );
	BOOST_CHECK_EQUAL( 2, value2);
	BOOST_CHECK_EQUAL( 2, value3);

	BOOST_CHECK( sched.run() );
	BOOST_CHECK( ! sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 1), sched.size() );
	BOOST_CHECK_EQUAL( 2, value2);
	BOOST_CHECK_EQUAL( 3, value3);

	BOOST_CHECK( sched.run() );
	BOOST_CHECK( sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 0), sched.size() );
	BOOST_CHECK_EQUAL( 2, value2);
	BOOST_CHECK_EQUAL( 3, value3);

	BOOST_CHECK( ! sched.run() );
	BOOST_CHECK( sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 0), sched.size() );
	BOOST_CHECK_EQUAL( 2, value2);
	BOOST_CHECK_EQUAL( 3, value3);
}

boost::unit_test::test_suite * init_unit_test_suite( int, char* [])
{
	boost::unit_test::test_suite * test =
		BOOST_TEST_SUITE("Boost.Tasklet: cancel test suite");

	test->add( BOOST_TEST_CASE( & test_case_1) );
	test->add( BOOST_TEST_CASE( & test_case_2) );

	return test;
}
