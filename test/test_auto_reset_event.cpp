
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

#include <boost/fiber/all.hpp>

int value = 0;

void wait_fn( boost::fibers::auto_reset_event & ev)
{
	ev.wait();
	++value;
}

void fn1()
{
	value = 0;
	boost::fibers::auto_reset_event ev;

	boost::fibers::fiber s1(
            boost::bind(
                wait_fn,
                boost::ref( ev) ) );
	BOOST_CHECK( s1);
	BOOST_CHECK_EQUAL( 0, value);

	boost::fibers::fiber s2(
            boost::bind(
                wait_fn,
                boost::ref( ev) ) );
	BOOST_CHECK( s2);
	BOOST_CHECK_EQUAL( 0, value);

	BOOST_CHECK( ! boost::fibers::run() );
	BOOST_CHECK( s1);
	BOOST_CHECK( s2);
	BOOST_CHECK_EQUAL( 0, value);

	ev.set();

	BOOST_CHECK( boost::fibers::run() );
	BOOST_CHECK( ! s1);
	BOOST_CHECK_EQUAL( 1, value);

	BOOST_CHECK( ! boost::fibers::run() );
	BOOST_CHECK( s2);
	BOOST_CHECK_EQUAL( 1, value);

	ev.set();

	BOOST_CHECK( boost::fibers::run() );
	BOOST_CHECK( ! s2);
	BOOST_CHECK_EQUAL( 2, value);
}

void fn2()
{
	value = 0;
	boost::fibers::auto_reset_event ev( true);

	boost::fibers::fiber s1(
            boost::bind(
                wait_fn,
                boost::ref( ev) ) );
	BOOST_CHECK( ! s1);
	BOOST_CHECK_EQUAL( 1, value);

	boost::fibers::fiber s2(
            boost::bind(
                wait_fn,
                boost::ref( ev) ) );
	BOOST_CHECK( s2);
	BOOST_CHECK_EQUAL( 1, value);

	BOOST_CHECK( ! boost::fibers::run() );
	BOOST_CHECK_EQUAL( 1, value);
	BOOST_CHECK( s2);

    ev.set();

	BOOST_CHECK( boost::fibers::run() );
	BOOST_CHECK_EQUAL( 2, value);
	BOOST_CHECK( ! s2);
}

void fn3()
{
	boost::fibers::auto_reset_event ev;

	BOOST_CHECK_EQUAL( false, ev.try_wait() );

	ev.set();

	BOOST_CHECK_EQUAL( true, ev.try_wait() );
	BOOST_CHECK_EQUAL( false, ev.try_wait() );
}

void do_wait_set()
{
    boost::fibers::fiber( fn1).join();
    fn1();
}

void do_wait_reset()
{
    boost::fibers::fiber( fn2).join();
    fn2();
}

void do_try_wait()
{
    boost::fibers::fiber( fn3).join();
    fn3();
}

void test_wait_set()
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    boost::fibers::fiber( do_wait_set).join();
}

void test_wait_reset()
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    boost::fibers::fiber( do_wait_reset).join();
}

void test_try_wait()
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    boost::fibers::fiber( do_try_wait).join();
}

boost::unit_test::test_suite * init_unit_test_suite( int, char* [])
{
    boost::unit_test::test_suite * test =
        BOOST_TEST_SUITE("Boost.Fiber: auto_reset_event test suite");

	test->add( BOOST_TEST_CASE( & test_wait_set) );
	test->add( BOOST_TEST_CASE( & test_wait_reset) );
	test->add( BOOST_TEST_CASE( & test_try_wait) );

	return test;
}
