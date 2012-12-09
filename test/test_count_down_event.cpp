
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

void wait_fn( stm::count_down_event & ev)
{
	ev.wait();
	++value;
}

void fn1()
{
	stm::count_down_event ev( 3);

	BOOST_CHECK_EQUAL( ev.initial(), ( std::size_t)3);
	BOOST_CHECK_EQUAL( ev.current(), ( std::size_t)3);

	ev.set();
	BOOST_CHECK_EQUAL( ev.initial(), ( std::size_t)3);
	BOOST_CHECK_EQUAL( ev.current(), ( std::size_t)2);

	ev.set();
	BOOST_CHECK_EQUAL( ev.initial(), ( std::size_t)3);
	BOOST_CHECK_EQUAL( ev.current(), ( std::size_t)1);

	ev.set();
	BOOST_CHECK_EQUAL( ev.initial(), ( std::size_t)3);
	BOOST_CHECK_EQUAL( ev.current(), ( std::size_t)0);

	ev.set();
	BOOST_CHECK_EQUAL( ev.initial(), ( std::size_t)3);
	BOOST_CHECK_EQUAL( ev.current(), ( std::size_t)0);
}

void fn2()
{
	value = 0;
	stm::count_down_event ev( 3);

	BOOST_CHECK_EQUAL( ev.initial(), ( std::size_t)3);
	BOOST_CHECK_EQUAL( ev.current(), ( std::size_t)3);

	stm::fiber s(
            boost::bind(
                wait_fn,
                boost::ref( ev) ) );
	BOOST_CHECK_EQUAL( 0, value);

	BOOST_CHECK( ! stm::run() );
	BOOST_CHECK_EQUAL( 0, value);

	ev.set();
	BOOST_CHECK( ! stm::run() );
	BOOST_CHECK_EQUAL( 0, value);

    ev.set();
	BOOST_CHECK( ! stm::run() );
	BOOST_CHECK_EQUAL( 0, value);

	ev.set();
	BOOST_CHECK( stm::run() );
	BOOST_CHECK_EQUAL( ev.initial(), ( std::size_t)3);
	BOOST_CHECK_EQUAL( ev.current(), ( std::size_t)0);
	BOOST_CHECK_EQUAL( 1, value);
}

void test_count_down()
{
    stm::fiber( fn1).join();
    fn1();
}

void test_wait()
{
    stm::fiber( fn2).join();
    fn2();
}

boost::unit_test::test_suite * init_unit_test_suite( int, char* [])
{
    boost::unit_test::test_suite * test =
        BOOST_TEST_SUITE("Boost.Stratified: count_down_event test suite");

	test->add( BOOST_TEST_CASE( & test_count_down) );
	test->add( BOOST_TEST_CASE( & test_wait) );

	return test;
}
