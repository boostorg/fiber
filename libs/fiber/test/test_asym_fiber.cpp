
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <sstream>
#include <string>

#include <boost/assert.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/thread.hpp>
#include <boost/utility.hpp>

#include <boost/fiber/all.hpp>

int value;

boost::asym_fiber gf;

void zero_args_fn()
{}

void one_args_fn( int)
{}

void two_args_fn( int, std::string const&)
{}

void set_value_fn( int i)
{ value = i; }

void increment_value_fn()
{
	while ( true)
	{
		++value;
		gf.yield();
	}
}

struct X
{
	void operator()()
	{}
};

struct Y
{
	void operator()( int i)
	{}
};

void increment( int k, int n)
{
	for ( int i = k; i < n; ++i)
	{
		gf.run();
		BOOST_CHECK_EQUAL( i + 1, value);
	}
}

void fn_first( int k, int n, boost::barrier & b)
{
	increment( k, n);
	b.wait();
}

void fn_last( int k, int n, boost::barrier & b)
{
	b.wait();
	increment( k, n);
}

void test_case_1()
{
	X x;
	boost::asym_fiber f1( & X::operator(), x, boost::asym_fiber::default_stacksize);
	BOOST_CHECK( f1);

	Y y;
	boost::asym_fiber f2( & Y::operator(), y, 7, boost::asym_fiber::default_stacksize);
	BOOST_CHECK( f2);
}

void test_case_2()
{
	boost::asym_fiber f1( one_args_fn, 10, boost::asym_fiber::default_stacksize);
	boost::asym_fiber f2;
	BOOST_CHECK( f1);
	BOOST_CHECK( ! f2);
}

void test_case_3()
{
	boost::asym_fiber f1( zero_args_fn, boost::asym_fiber::default_stacksize);
	boost::asym_fiber f2( one_args_fn, 1, boost::asym_fiber::default_stacksize);
	boost::asym_fiber f3( two_args_fn, 1, "abc", boost::asym_fiber::default_stacksize);
}

void test_case_4()
{
	boost::asym_fiber f1( zero_args_fn, boost::asym_fiber::default_stacksize);
	f1 = boost::asym_fiber( zero_args_fn, boost::asym_fiber::default_stacksize);
	boost::asym_fiber f2( one_args_fn, 1, boost::asym_fiber::default_stacksize);
	f2 = boost::asym_fiber( one_args_fn, 1, boost::asym_fiber::default_stacksize);
}

void test_case_5()
{
	boost::asym_fiber f1( one_args_fn, 10, boost::asym_fiber::default_stacksize);
	BOOST_CHECK( f1);
	boost::asym_fiber f2( boost::move( f1) );
	BOOST_CHECK( ! f1);
	BOOST_CHECK( f2);
	boost::asym_fiber f3;
	BOOST_CHECK( ! f3);
	f3 = f2;
	BOOST_CHECK( f3);
	BOOST_CHECK_EQUAL( f2, f3);
}

void test_case_6()
{
	boost::asym_fiber f1( zero_args_fn, boost::asym_fiber::default_stacksize);
	boost::asym_fiber f2( zero_args_fn, boost::asym_fiber::default_stacksize);
	boost::asym_fiber f3;
	BOOST_CHECK( f1);
	BOOST_CHECK( f2);
	BOOST_CHECK( ! f3);

	BOOST_CHECK( f1 != f2);
	BOOST_CHECK( f1 != f3);
	BOOST_CHECK( f2 != f3);

	std::ostringstream os1;
	os1 << f1.get_id();
	std::ostringstream os2;
	os2 << f2.get_id();
	std::ostringstream os3;
	os3 << f3.get_id();

	std::string not_a_fiber("{not-a-fiber}");
	BOOST_CHECK( os1.str() != os2.str() );
	BOOST_CHECK( os1.str() != os3.str() );
	BOOST_CHECK( os2.str() != os3.str() );
	BOOST_CHECK( os1.str() != not_a_fiber);
	BOOST_CHECK( os2.str() != not_a_fiber);
	BOOST_CHECK( os3.str() == not_a_fiber);
}

void test_case_7()
{
	boost::asym_fiber f1( zero_args_fn, boost::asym_fiber::default_stacksize);
	boost::asym_fiber f2( zero_args_fn, boost::asym_fiber::default_stacksize);

	boost::asym_fiber::id id1 = f1.get_id();
	boost::asym_fiber::id id2 = f2.get_id();

	f1.swap( f2);

	BOOST_CHECK_EQUAL( f1.get_id(), id2);
	BOOST_CHECK_EQUAL( f2.get_id(), id1);
}

void test_case_8()
{
	value = 0;

	boost::asym_fiber f( set_value_fn, 7, boost::asym_fiber::default_stacksize);

	BOOST_CHECK_EQUAL( 0, value);

	f.run();

	BOOST_CHECK_EQUAL( 7, value);
}

void test_case_9()
{
	value = 0;

	gf = boost::asym_fiber( increment_value_fn, boost::asym_fiber::default_stacksize);

	BOOST_CHECK_EQUAL( 0, value);

	for ( int i = 0; i < 7; ++i)
	{
		gf.run();
		BOOST_CHECK_EQUAL( i + 1, value);
	}
}

void test_case_10()
{
	value = 0;

	gf = boost::asym_fiber( increment_value_fn, boost::asym_fiber::default_stacksize);

	BOOST_CHECK_EQUAL( 0, value);

	boost::barrier b( 2);
	boost::thread t1( fn_first, 0, 5, boost::ref( b) );
	boost::thread t2( fn_last, 5, 7, boost::ref( b) );
	t1.join();
	t2.join();

	BOOST_CHECK_EQUAL( 7, value);
}

boost::unit_test::test_suite * init_unit_test_suite( int, char* [])
{
	boost::unit_test::test_suite * test =
		BOOST_TEST_SUITE("Boost.Fiber: fiber test suite");

	test->add( BOOST_TEST_CASE( & test_case_1) );
	test->add( BOOST_TEST_CASE( & test_case_2) );
	test->add( BOOST_TEST_CASE( & test_case_3) );
	test->add( BOOST_TEST_CASE( & test_case_4) );
	test->add( BOOST_TEST_CASE( & test_case_5) );
	test->add( BOOST_TEST_CASE( & test_case_6) );
	test->add( BOOST_TEST_CASE( & test_case_7) );
	test->add( BOOST_TEST_CASE( & test_case_8) );
	test->add( BOOST_TEST_CASE( & test_case_9) );
	test->add( BOOST_TEST_CASE( & test_case_10) );

	return test;
}
