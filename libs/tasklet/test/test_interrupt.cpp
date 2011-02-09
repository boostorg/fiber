
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <sstream>
#include <stdexcept>
#include <string>

#include <boost/test/unit_test.hpp>
#include <boost/utility.hpp>

#include <boost/tasklet.hpp>

int value1 = 0;
int value2 = 0;
bool interrupted = false;

void fn_1()
{
	try
	{
		for ( int i = 0; i < 5; ++i)
		{
			++value1;
			boost::this_tasklet::interruption_point();
			boost::this_tasklet::yield();
		}
	}
	catch ( boost::tasklets::tasklet_interrupted const&)
	{ interrupted = true; }
}

void fn_2()
{
	boost::this_tasklet::disable_interruption disabler;
	if ( boost::this_tasklet::interruption_enabled() )
		throw std::logic_error("interruption enabled");
	for ( int i = 0; i < 5; ++i)
	{
		++value1;
		boost::this_tasklet::interruption_point();
		boost::this_tasklet::yield();
	}
}

void fn_3()
{
	try
	{
		boost::this_tasklet::disable_interruption disabler;
		if ( boost::this_tasklet::interruption_enabled() )
			throw std::logic_error("interruption enabled");
		for ( int i = 0; i < 5; ++i)
		{
			++value1;
			boost::this_tasklet::restore_interruption restorer( disabler);
			boost::this_tasklet::interruption_point();
			boost::this_tasklet::yield();
		}
	}
	catch ( boost::tasklets::tasklet_interrupted const&)
	{ interrupted = true; }
}

void fn_5( boost::tasklet f)
{
	for ( int i = 0; i < 5; ++i)
	{
		++value2;
		if ( i == 1) f.interrupt();
		if ( i >= 1)
		{
			if ( ! f.interruption_requested() )
				throw std::logic_error("");
		}
		boost::this_tasklet::yield();
	}
}

void test_case_1()
{
	value1 = 0;
	value2 = 0;
	interrupted = false;

	boost::tasklets::scheduler<> sched;

	boost::tasklet f( fn_1, boost::tasklet::default_stacksize, boost::protected_stack_allocator());
	sched.submit_tasklet( f);
	sched.submit_tasklet(
		boost::tasklet(
			fn_5, f, boost::tasklet::default_stacksize, boost::protected_stack_allocator()) );

	BOOST_CHECK_EQUAL( 0, value1);
	BOOST_CHECK_EQUAL( 0, value2);

	BOOST_CHECK( sched.run() );
	BOOST_CHECK( ! sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 2), sched.size() );
	BOOST_CHECK_EQUAL( false, interrupted);
	BOOST_CHECK_EQUAL( 1, value1);
	BOOST_CHECK_EQUAL( 0, value2);

	BOOST_CHECK( sched.run() );
	BOOST_CHECK( ! sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 2), sched.size() );
	BOOST_CHECK_EQUAL( false, interrupted);
	BOOST_CHECK_EQUAL( 1, value1);
	BOOST_CHECK_EQUAL( 1, value2);

	BOOST_CHECK( sched.run() );
	BOOST_CHECK( ! sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 2), sched.size() );
	BOOST_CHECK_EQUAL( false, interrupted);
	BOOST_CHECK_EQUAL( 2, value1);
	BOOST_CHECK_EQUAL( 1, value2);

	BOOST_CHECK( sched.run() );
	BOOST_CHECK( ! sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 2), sched.size() );
	BOOST_CHECK_EQUAL( false, interrupted);
	BOOST_CHECK_EQUAL( 2, value1);
	BOOST_CHECK_EQUAL( 2, value2);

	BOOST_CHECK( sched.run() );
	BOOST_CHECK( ! sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 1), sched.size() );
	BOOST_CHECK_EQUAL( true, interrupted);
	BOOST_CHECK_EQUAL( 3, value1);
	BOOST_CHECK_EQUAL( 2, value2);

	BOOST_CHECK( sched.run() );
	BOOST_CHECK( ! sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 1), sched.size() );
	BOOST_CHECK_EQUAL( true, interrupted);
	BOOST_CHECK_EQUAL( 3, value1);
	BOOST_CHECK_EQUAL( 3, value2);

	BOOST_CHECK( sched.run() );
	BOOST_CHECK( ! sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 1), sched.size() );
	BOOST_CHECK_EQUAL( true, interrupted);
	BOOST_CHECK_EQUAL( 3, value1);
	BOOST_CHECK_EQUAL( 4, value2);

	BOOST_CHECK( sched.run() );
	BOOST_CHECK( ! sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 1), sched.size() );
	BOOST_CHECK_EQUAL( true, interrupted);
	BOOST_CHECK_EQUAL( 3, value1);
	BOOST_CHECK_EQUAL( 5, value2);

	BOOST_CHECK( sched.run() );
	BOOST_CHECK( sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 0), sched.size() );
	BOOST_CHECK_EQUAL( true, interrupted);
	BOOST_CHECK_EQUAL( 3, value1);
	BOOST_CHECK_EQUAL( 5, value2);

	BOOST_CHECK( ! sched.run() );
	BOOST_CHECK( sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 0), sched.size() );
	BOOST_CHECK_EQUAL( true, interrupted);
	BOOST_CHECK_EQUAL( 3, value1);
	BOOST_CHECK_EQUAL( 5, value2);
}

void test_case_2()
{
	value1 = 0;
	value2 = 0;
	interrupted = false;

	boost::tasklets::scheduler<> sched;

	boost::tasklet f( fn_2, boost::tasklet::default_stacksize, boost::protected_stack_allocator());
	sched.submit_tasklet( f);
	sched.submit_tasklet(
		boost::tasklet(
			fn_5, f, boost::tasklet::default_stacksize, boost::protected_stack_allocator()) );

	BOOST_CHECK_EQUAL( 0, value1);
	BOOST_CHECK_EQUAL( 0, value2);
	BOOST_CHECK_EQUAL( false, interrupted);
	BOOST_CHECK( ! sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 2), sched.size() );

	for (;;)
	{
		while ( sched.run() );
		if ( sched.empty() ) break;
	}

	BOOST_CHECK( ! sched.run() );
	BOOST_CHECK( sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 0), sched.size() );
	BOOST_CHECK_EQUAL( 5, value1);
	BOOST_CHECK_EQUAL( 5, value2);
	BOOST_CHECK_EQUAL( false, interrupted);
}

void test_case_3()
{
	value1 = 0;
	value2 = 0;
	interrupted = false;

	boost::tasklets::scheduler<> sched;

	boost::tasklet f( fn_3, boost::tasklet::default_stacksize, boost::protected_stack_allocator());
	sched.submit_tasklet( f);
	sched.submit_tasklet(
		boost::tasklet(
			fn_5, f, boost::tasklet::default_stacksize, boost::protected_stack_allocator()) );

	BOOST_CHECK_EQUAL( 0, value1);
	BOOST_CHECK_EQUAL( 0, value2);
	BOOST_CHECK_EQUAL( false, interrupted);
	BOOST_CHECK( ! sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 2), sched.size() );

	for (;;)
	{
		while ( sched.run() );
		if ( sched.empty() ) break;
	}

	BOOST_CHECK( ! sched.run() );
	BOOST_CHECK( sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 0), sched.size() );
	BOOST_CHECK_EQUAL( 3, value1);
	BOOST_CHECK_EQUAL( 5, value2);
	BOOST_CHECK_EQUAL( true, interrupted);
}

boost::unit_test::test_suite * init_unit_test_suite( int, char* [])
{
	boost::unit_test::test_suite * test =
		BOOST_TEST_SUITE("Boost.Tasklet: interrupt test suite");

	test->add( BOOST_TEST_CASE( & test_case_1) );
	test->add( BOOST_TEST_CASE( & test_case_2) );
	test->add( BOOST_TEST_CASE( & test_case_3) );

	return test;
}
