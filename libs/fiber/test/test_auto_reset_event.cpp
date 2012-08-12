
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

namespace stm = boost::fibers;
namespace this_stm = boost::this_fiber;

int value = 0;

void wait_fn( stm::auto_reset_event & ev)
{
	ev.wait();
	++value;
}

void fn1()
{
	value = 0;
	stm::auto_reset_event ev;

	stm::fiber s1(
        stm::spawn(
            boost::bind(
                wait_fn,
                boost::ref( ev) ) ) );
	BOOST_CHECK( ! s1.is_complete() );
	BOOST_CHECK_EQUAL( 0, value);

	stm::fiber s2(
        stm::spawn(
            boost::bind(
                wait_fn,
                boost::ref( ev) ) ) );
	BOOST_CHECK( ! s2.is_complete() );
	BOOST_CHECK_EQUAL( 0, value);

	BOOST_CHECK( ! stm::run() );
	BOOST_CHECK( ! s1.is_complete() );
	BOOST_CHECK( ! s2.is_complete() );
	BOOST_CHECK_EQUAL( 0, value);

	ev.set();

	BOOST_CHECK( stm::run() );
	BOOST_CHECK( s1.is_complete() );
	BOOST_CHECK_EQUAL( 1, value);

	BOOST_CHECK( ! stm::run() );
	BOOST_CHECK( ! s2.is_complete() );
	BOOST_CHECK_EQUAL( 1, value);

	ev.set();

	BOOST_CHECK( stm::run() );
	BOOST_CHECK( s2.is_complete() );
	BOOST_CHECK_EQUAL( 2, value);
}

void fn2()
{
	value = 0;
	stm::auto_reset_event ev( true);

	stm::fiber s1(
        stm::spawn(
            boost::bind(
                wait_fn,
                boost::ref( ev) ) ) );
	BOOST_CHECK( s1.is_complete() );
	BOOST_CHECK_EQUAL( 1, value);

	stm::fiber s2(
        stm::spawn(
            boost::bind(
                wait_fn,
                boost::ref( ev) ) ) );
	BOOST_CHECK( ! s2.is_complete() );
	BOOST_CHECK_EQUAL( 1, value);

	BOOST_CHECK( ! stm::run() );
	BOOST_CHECK_EQUAL( 1, value);
	BOOST_CHECK( ! s2.is_complete() );

    ev.set();

	BOOST_CHECK( stm::run() );
	BOOST_CHECK_EQUAL( 2, value);
	BOOST_CHECK( s2.is_complete() );
}

void fn3()
{
	stm::auto_reset_event ev;

	BOOST_CHECK_EQUAL( false, ev.try_wait() );

	ev.set();

	BOOST_CHECK_EQUAL( true, ev.try_wait() );
	BOOST_CHECK_EQUAL( false, ev.try_wait() );
}

void test_wait_set()
{
    stm::spawn( fn1).join();
    fn1();
}

void test_wait_reset()
{
    stm::spawn( fn2).join();
    fn2();
}

void test_try_wait()
{
    stm::spawn( fn3).join();
    fn3();
}

boost::unit_test::test_suite * init_unit_test_suite( int, char* [])
{
    boost::unit_test::test_suite * test =
        BOOST_TEST_SUITE("Boost.Stratified: auto_reset_event test suite");

	test->add( BOOST_TEST_CASE( & test_wait_set) );
	test->add( BOOST_TEST_CASE( & test_wait_reset) );
	test->add( BOOST_TEST_CASE( & test_try_wait) );

	return test;
}
