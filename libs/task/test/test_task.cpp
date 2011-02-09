
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <vector>

#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/function.hpp>
#include <boost/ref.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/thread.hpp>
#include <boost/utility.hpp>

#include <boost/task.hpp>

#include "test_functions.hpp"

namespace pt = boost::posix_time;
namespace tsk = boost::tasks;

void zero_args_fn() {}
int one_arg_fn( int i) { return i; }
int two_args_fn( int i, std::string const& s) { return i; }

// check vaild task
void test_case_1()
{
	tsk::task< int > t1( fibonacci_fn, 10);
	tsk::task< int > t2;
	BOOST_CHECK( t1);
	BOOST_CHECK( ! t2);
}

// check make_task
void test_case_2()
{
	tsk::task< void > t1;
	BOOST_CHECK( ! t1);
	t1 = tsk::make_task( zero_args_fn);
	BOOST_CHECK( t1);
	tsk::task< int > t2 = tsk::make_task( one_arg_fn, 1);
	BOOST_CHECK( t2);
	tsk::task< int > t3;
	BOOST_CHECK( ! t3);
	t3 = tsk::make_task( two_args_fn, 1, "abc");
	BOOST_CHECK( t3);
}

// check moved task
void test_case_3()
{
	tsk::task< int > t1( fibonacci_fn, 10);
	BOOST_CHECK( t1);
	tsk::task< int > t2( boost::move( t1) );
	BOOST_CHECK( ! t1);
	BOOST_CHECK_THROW( t1(), tsk::task_moved);
	BOOST_CHECK_NO_THROW( t2() );
}

// check execute twice
void test_case_4()
{
	tsk::task< int > t1( fibonacci_fn, 10);
	BOOST_CHECK_NO_THROW( t1() );
	BOOST_CHECK_THROW( t1(), tsk::task_already_executed);
}

// check swap
void test_case_5()
{
	tsk::task< int > t1( fibonacci_fn, 10);
	tsk::task< int > t2;
	BOOST_CHECK_NO_THROW( t1() );
	BOOST_CHECK_THROW( t2(), tsk::task_moved);
	t1.swap( t2);
	BOOST_CHECK_THROW( t1(), tsk::task_moved);
	BOOST_CHECK_THROW( t2(), tsk::task_already_executed);
}

boost::unit_test::test_suite * init_unit_test_suite( int, char* [])
{
	boost::unit_test::test_suite * test =
		BOOST_TEST_SUITE("Boost.Task: task test suite");

	test->add( BOOST_TEST_CASE( & test_case_1) );
	test->add( BOOST_TEST_CASE( & test_case_2) );
	test->add( BOOST_TEST_CASE( & test_case_3) );
	test->add( BOOST_TEST_CASE( & test_case_4) );
	test->add( BOOST_TEST_CASE( & test_case_5) );

	return test;
}
