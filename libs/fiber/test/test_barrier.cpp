
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <sstream>
#include <string>

#include <boost/test/unit_test.hpp>
#include <boost/utility.hpp>

#include <boost/fiber/all.hpp>

namespace stm = boost::fibers;
namespace this_stm = boost::this_fiber;

int value1 = 0;
int value2 = 0;

void fn1( stm::barrier & b)
{
	++value1;
	this_stm::yield();

	b.wait();

	++value1;
	this_stm::yield();
	++value1;
	this_stm::yield();
	++value1;
	this_stm::yield();
	++value1;
}

void fn2( stm::barrier & b)
{
	++value2;
	this_stm::yield();
	++value2;
	this_stm::yield();
	++value2;
	this_stm::yield();

	b.wait();

	++value2;
	this_stm::yield();
	++value2;
}

void test_barrier()
{
	value1 = 0;
	value2 = 0;

	stm::barrier b( 2);
    stm::fiber s1(
        stm::spawn(
            boost::bind(
			    fn1, boost::ref( b) ) ) );
	BOOST_CHECK( ! s1.is_complete() );
	BOOST_CHECK_EQUAL( 1, value1);

    stm::fiber s2(
        stm::spawn(
            boost::bind(
			    fn2, boost::ref( b) ) ) );
	BOOST_CHECK( ! s2.is_complete() );
	BOOST_CHECK_EQUAL( 1, value2);

	BOOST_CHECK( stm::run() );
	BOOST_CHECK( ! s1.is_complete() );
	BOOST_CHECK( ! s2.is_complete() );
	BOOST_CHECK_EQUAL( 1, value1);
	BOOST_CHECK_EQUAL( 1, value2);

	BOOST_CHECK( stm::run() );
	BOOST_CHECK( ! s1.is_complete() );
	BOOST_CHECK( ! s2.is_complete() );
	BOOST_CHECK_EQUAL( 1, value1);
	BOOST_CHECK_EQUAL( 2, value2);

	BOOST_CHECK( stm::run() );
	BOOST_CHECK( ! s1.is_complete() );
	BOOST_CHECK( ! s2.is_complete() );
	BOOST_CHECK_EQUAL( 1, value1);
	BOOST_CHECK_EQUAL( 3, value2);

	BOOST_CHECK( stm::run() );
	BOOST_CHECK( ! s1.is_complete() );
	BOOST_CHECK( ! s2.is_complete() );
	BOOST_CHECK_EQUAL( 1, value1);
	BOOST_CHECK_EQUAL( 4, value2);

	BOOST_CHECK( stm::run() );
	BOOST_CHECK( ! s1.is_complete() );
	BOOST_CHECK( ! s2.is_complete() );
	BOOST_CHECK_EQUAL( 2, value1);
	BOOST_CHECK_EQUAL( 4, value2);

	BOOST_CHECK( stm::run() );
	BOOST_CHECK( ! s1.is_complete() );
	BOOST_CHECK( s2.is_complete() );
	BOOST_CHECK_EQUAL( 2, value1);
	BOOST_CHECK_EQUAL( 5, value2);

	BOOST_CHECK( stm::run() );
	BOOST_CHECK( ! s1.is_complete() );
	BOOST_CHECK( s2.is_complete() );
	BOOST_CHECK_EQUAL( 3, value1);
	BOOST_CHECK_EQUAL( 5, value2);

	BOOST_CHECK( stm::run() );
	BOOST_CHECK( ! s1.is_complete() );
	BOOST_CHECK( s2.is_complete() );
	BOOST_CHECK_EQUAL( 4, value1);
	BOOST_CHECK_EQUAL( 5, value2);

	BOOST_CHECK( stm::run() );
	BOOST_CHECK( s1.is_complete() );
	BOOST_CHECK( s2.is_complete() );
	BOOST_CHECK_EQUAL( 5, value1);
	BOOST_CHECK_EQUAL( 5, value2);

	BOOST_CHECK( ! stm::run() );
	BOOST_CHECK( s1.is_complete() );
	BOOST_CHECK( s2.is_complete() );
	BOOST_CHECK_EQUAL( 5, value1);
	BOOST_CHECK_EQUAL( 5, value2);
}

boost::unit_test::test_suite * init_unit_test_suite( int, char* [])
{
    boost::unit_test::test_suite * test =
        BOOST_TEST_SUITE("Boost.Stratified: barrier test suite");

	test->add( BOOST_TEST_CASE( & test_barrier) );

	return test;
}
