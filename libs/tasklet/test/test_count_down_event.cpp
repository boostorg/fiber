
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
#include <boost/cstdint.hpp>
#include <boost/function.hpp>
#include <boost/ref.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/utility.hpp>

#include <boost/tasklet.hpp>

int value = 0;

void wait_fn( boost::tasklets::count_down_event & ev)
{
	ev.wait();
	++value;
}

void test_case_1()
{
	unsigned int n = 3;
	boost::tasklets::scheduler<> sched;
	boost::tasklets::count_down_event ev( n);

	BOOST_CHECK_EQUAL( ev.initial(), n);
	BOOST_CHECK_EQUAL( ev.current(), n);

	ev.set();
	BOOST_CHECK_EQUAL( ev.initial(), n);
	BOOST_CHECK_EQUAL( ev.current(), 2);

	ev.set();
	BOOST_CHECK_EQUAL( ev.initial(), n);
	BOOST_CHECK_EQUAL( ev.current(), 1);

	ev.set();
	BOOST_CHECK_EQUAL( ev.initial(), n);
	BOOST_CHECK_EQUAL( ev.current(), 0);

	ev.set();
	BOOST_CHECK_EQUAL( ev.initial(), n);
	BOOST_CHECK_EQUAL( ev.current(), 0);
}

void test_case_2()
{
	value = 0;
	unsigned int n = 3;
	boost::tasklets::scheduler<> sched;
	boost::tasklets::count_down_event ev( n);

	BOOST_CHECK_EQUAL( ev.initial(), n);
	BOOST_CHECK_EQUAL( ev.current(), n);

	sched.submit_tasklet(
		boost::tasklet(
			wait_fn,
			boost::ref( ev),
			boost::tasklet::default_stacksize, boost::protected_stack_allocator()) );

	ev.set();
	BOOST_CHECK( sched.run() );
	BOOST_CHECK( value != 1);

	ev.set();
	BOOST_CHECK( sched.run() );
	BOOST_CHECK( value != 1);

	ev.set();
	BOOST_CHECK( sched.run() );
	BOOST_CHECK_EQUAL( ev.initial(), n);
	BOOST_CHECK_EQUAL( ev.current(), 0);
	BOOST_CHECK_EQUAL( 1, value);
}

boost::unit_test::test_suite * init_unit_test_suite( int, char* [])
{
	boost::unit_test::test_suite * test =
		BOOST_TEST_SUITE("Boost.Task: spin-count-down-event test suite");

	test->add( BOOST_TEST_CASE( & test_case_1) );
	test->add( BOOST_TEST_CASE( & test_case_2) );

	return test;
}
