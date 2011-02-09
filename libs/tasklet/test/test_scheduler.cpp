
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <sstream>
#include <string>

#include <boost/ref.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/utility.hpp>

#include <boost/tasklet.hpp>

int value1 = 0;
int value2 = 0;
int value3 = 0;

void zero_args_fn() {}
void one_args_fn( int) {}

void value1_fn()
{ value1 = 1; }

void value2_fn()
{ value2 = 1; }

void value3_fn()
{ value3 = 1; }

void yield1_fn()
{
	for ( int i = 0; i < 5; ++i)
	{
		++value1;
		boost::this_tasklet::yield();
	}
}

void yield2_fn()
{
	for ( int i = 0; i < 5; ++i)
	{
		++value2;
		boost::this_tasklet::yield();
	}
}

void test_case_1()
{
	value1 = 0;
	value2 = 0;

	boost::tasklets::scheduler<> sched;

	BOOST_CHECK( sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 0), sched.size() );
	BOOST_CHECK( ! sched.run() );

	boost::tasklet f( boost::tasklet( zero_args_fn, boost::tasklet::default_stacksize, boost::protected_stack_allocator()) );
	BOOST_CHECK( ! f.is_alive() );
	sched.submit_tasklet( f);
	BOOST_CHECK( f.is_alive() );
	BOOST_CHECK( ! sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 1), sched.size() );

	sched.submit_tasklet(
		boost::tasklet( zero_args_fn, boost::tasklet::default_stacksize, boost::protected_stack_allocator()) );
	BOOST_CHECK( ! sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 2), sched.size() );

	BOOST_CHECK( sched.run() );
	BOOST_CHECK( ! sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 1), sched.size() );

	BOOST_CHECK( sched.run() );
	BOOST_CHECK( sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 0), sched.size() );

	BOOST_CHECK( ! f.is_alive() );
}

void test_case_2()
{
	value1 = 0;
	value2 = 0;

	boost::tasklets::scheduler<> sched;

	sched.submit_tasklet(
		boost::tasklet(
			value1_fn, boost::tasklet::default_stacksize, boost::protected_stack_allocator()) );
	sched.submit_tasklet(
		boost::tasklet(
			value2_fn, boost::tasklet::default_stacksize, boost::protected_stack_allocator()) );
	BOOST_CHECK( ! sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 2), sched.size() );
	BOOST_CHECK_EQUAL( 0, value1);
	BOOST_CHECK_EQUAL( 0, value2);

	BOOST_CHECK( sched.run() );
	BOOST_CHECK( ! sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 1), sched.size() );
	BOOST_CHECK_EQUAL( 1, value1);	
	BOOST_CHECK_EQUAL( 0, value2);

	BOOST_CHECK( sched.run() );
	BOOST_CHECK( sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 0), sched.size() );
	BOOST_CHECK_EQUAL( 1, value1);	
	BOOST_CHECK_EQUAL( 1, value2);

	BOOST_CHECK( ! sched.run() );
	BOOST_CHECK( sched.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 0), sched.size() );
	BOOST_CHECK_EQUAL( 1, value1);	
	BOOST_CHECK_EQUAL( 1, value2);
}

void test_case_3()
{
	value1 = 0;
	value2 = 0;

	boost::tasklets::scheduler<> sched1, sched2;

	boost::tasklet f( & yield1_fn, boost::tasklet::default_stacksize, boost::protected_stack_allocator()); 
	boost::tasklet::id id = f.get_id();
	sched1.submit_tasklet( f);
	sched2.submit_tasklet(
		boost::tasklet(
			& yield2_fn, boost::tasklet::default_stacksize, boost::protected_stack_allocator()) );

	BOOST_CHECK( ! sched1.empty() );
	BOOST_CHECK( ! sched2.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 1), sched1.size() );
	BOOST_CHECK_EQUAL( std::size_t( 1), sched2.size() );
	BOOST_CHECK_EQUAL( 0, value1);
	BOOST_CHECK_EQUAL( 0, value2);

	BOOST_CHECK( sched1.run() );
	BOOST_CHECK( ! sched1.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 1), sched1.size() );
	BOOST_CHECK_EQUAL( 1, value1);	
	BOOST_CHECK_EQUAL( 0, value2);

	BOOST_CHECK( sched2.run() );
	BOOST_CHECK( ! sched2.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 1), sched2.size() );
	BOOST_CHECK_EQUAL( 1, value1);	
	BOOST_CHECK_EQUAL( 1, value2);

	BOOST_CHECK( sched1.run() );
	BOOST_CHECK( ! sched1.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 1), sched1.size() );
	BOOST_CHECK_EQUAL( 2, value1);	
	BOOST_CHECK_EQUAL( 1, value2);

	BOOST_CHECK( sched2.run() );
	BOOST_CHECK( ! sched2.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 1), sched2.size() );
	BOOST_CHECK_EQUAL( 2, value1);	
	BOOST_CHECK_EQUAL( 2, value2);

	sched2.migrate_tasklet( f);
	BOOST_CHECK_EQUAL( std::size_t( 0), sched1.size() );
	BOOST_CHECK_EQUAL( std::size_t( 2), sched2.size() );

	BOOST_CHECK( ! sched1.run() );
	BOOST_CHECK( sched1.empty() );

	BOOST_CHECK( sched2.run() );
	BOOST_CHECK( ! sched2.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 2), sched2.size() );
	BOOST_CHECK_EQUAL( 2, value1);	
	BOOST_CHECK_EQUAL( 3, value2);

	BOOST_CHECK( sched2.run() );
	BOOST_CHECK( ! sched2.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 2), sched2.size() );
	BOOST_CHECK_EQUAL( 3, value1);	
	BOOST_CHECK_EQUAL( 3, value2);
}

void test_case_4()
{
	value1 = 0;
	value2 = 0;

	boost::tasklets::scheduler<> sched1, sched2;

	boost::tasklet f( & yield1_fn, boost::tasklet::default_stacksize, boost::protected_stack_allocator()); 
	boost::tasklet::id id = f.get_id();
	sched1.submit_tasklet( f);
	sched2.submit_tasklet(
		boost::tasklet(
			& yield2_fn, boost::tasklet::default_stacksize, boost::protected_stack_allocator()) );

	BOOST_CHECK( ! sched1.empty() );
	BOOST_CHECK( ! sched2.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 1), sched1.size() );
	BOOST_CHECK_EQUAL( std::size_t( 1), sched2.size() );
	BOOST_CHECK_EQUAL( 0, value1);
	BOOST_CHECK_EQUAL( 0, value2);

	BOOST_CHECK( sched1.run() );
	BOOST_CHECK( ! sched1.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 1), sched1.size() );
	BOOST_CHECK_EQUAL( 1, value1);	
	BOOST_CHECK_EQUAL( 0, value2);

	BOOST_CHECK( sched2.run() );
	BOOST_CHECK( ! sched2.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 1), sched2.size() );
	BOOST_CHECK_EQUAL( 1, value1);	
	BOOST_CHECK_EQUAL( 1, value2);

	BOOST_CHECK( sched1.run() );
	BOOST_CHECK( ! sched1.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 1), sched1.size() );
	BOOST_CHECK_EQUAL( 2, value1);	
	BOOST_CHECK_EQUAL( 1, value2);

	BOOST_CHECK( sched2.run() );
	BOOST_CHECK( ! sched2.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 1), sched2.size() );
	BOOST_CHECK_EQUAL( 2, value1);	
	BOOST_CHECK_EQUAL( 2, value2);

	sched2.migrate_tasklet( f);
	BOOST_CHECK_EQUAL( std::size_t( 0), sched1.size() );
	BOOST_CHECK_EQUAL( std::size_t( 2), sched2.size() );

	BOOST_CHECK( ! sched1.run() );
	BOOST_CHECK( sched1.empty() );

	BOOST_CHECK( sched2.run() );
	BOOST_CHECK( ! sched2.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 2), sched2.size() );
	BOOST_CHECK_EQUAL( 2, value1);	
	BOOST_CHECK_EQUAL( 3, value2);

	BOOST_CHECK( sched2.run() );
	BOOST_CHECK( ! sched2.empty() );
	BOOST_CHECK_EQUAL( std::size_t( 2), sched2.size() );
	BOOST_CHECK_EQUAL( 3, value1);	
	BOOST_CHECK_EQUAL( 3, value2);
}

boost::unit_test::test_suite * init_unit_test_suite( int, char* [])
{
	boost::unit_test::test_suite * test =
		BOOST_TEST_SUITE("Boost.Tasklet: scheduler test suite");

	test->add( BOOST_TEST_CASE( & test_case_1) );
	test->add( BOOST_TEST_CASE( & test_case_2) );
	test->add( BOOST_TEST_CASE( & test_case_3) );
	test->add( BOOST_TEST_CASE( & test_case_4) );

	return test;
}
