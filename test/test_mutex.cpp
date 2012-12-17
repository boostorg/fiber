
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// This test is based on the tests of Boost.Thread 

#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <vector>

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/ref.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/utility.hpp>

#include <boost/fiber/all.hpp>

namespace stm = boost::fibers;
namespace this_stm = boost::this_fiber;

template< typename M >
struct test_lock
{
    typedef M mutex_type;
    typedef typename M::scoped_lock lock_type;

    void operator()()
    {
        mutex_type mtx;

        // Test the lock's constructors.
        {
            lock_type lk(mtx, boost::defer_lock);
            BOOST_CHECK(!lk);
        }
        lock_type lk(mtx);
        BOOST_CHECK(lk ? true : false);

        // Test the lock and unlock methods.
        lk.unlock();
        BOOST_CHECK(!lk);
        lk.lock();
        BOOST_CHECK(lk ? true : false);
    }
};

void do_test_mutex()
{
    test_lock< stm::mutex >()();
}

int value1 = 0;
int value2 = 0;

void fn1( stm::mutex & mtx)
{
	stm::mutex::scoped_lock lk( mtx);
	++value1;
	for ( int i = 0; i < 3; ++i)
		this_stm::yield();
}

void fn2( stm::mutex & mtx)
{
	++value2;
	stm::mutex::scoped_lock lk( mtx);
	++value2;
}

void test_locking()
{
    stm::default_scheduler ds;
    stm::scheduler::replace( & ds);

    stm::fiber s( & do_test_mutex);
    BOOST_ASSERT( ! s);
	BOOST_ASSERT( ! stm::run() );
}

void test_exclusive()
{
    stm::default_scheduler ds;
    stm::scheduler::replace( & ds);

    value1 = 0;
    value2 = 0;
	BOOST_CHECK_EQUAL( 0, value1);
	BOOST_CHECK_EQUAL( 0, value2);

	stm::mutex mtx;
    stm::fiber s1(
		boost::bind( & fn1, boost::ref( mtx) ) );
    stm::fiber s2(
		boost::bind( & fn2, boost::ref( mtx) ) );
    BOOST_ASSERT( s1);
    BOOST_ASSERT( s2);
	BOOST_CHECK_EQUAL( 1, value1);
	BOOST_CHECK_EQUAL( 1, value2);

	BOOST_CHECK( stm::run() );
    BOOST_ASSERT( s1);
    BOOST_ASSERT( s2);
	BOOST_CHECK_EQUAL( 1, value1);
	BOOST_CHECK_EQUAL( 1, value2);

	BOOST_CHECK( stm::run() );
    BOOST_ASSERT( s1);
    BOOST_ASSERT( s2);
	BOOST_CHECK_EQUAL( 1, value1);
	BOOST_CHECK_EQUAL( 1, value2);

	BOOST_CHECK( stm::run() );
    BOOST_ASSERT( ! s1);
    BOOST_ASSERT( s2);
	BOOST_CHECK_EQUAL( 1, value1);
	BOOST_CHECK_EQUAL( 1, value2);

	BOOST_CHECK( stm::run() );
	BOOST_CHECK_EQUAL( 1, value1);
	BOOST_CHECK_EQUAL( 2, value2);

	BOOST_CHECK( ! stm::run() );
	BOOST_CHECK_EQUAL( 1, value1);
	BOOST_CHECK_EQUAL( 2, value2);
}

boost::unit_test::test_suite * init_unit_test_suite( int, char* [])
{
    boost::unit_test::test_suite * test =
        BOOST_TEST_SUITE("Boost.Stratified: mutex test suite");

    test->add( BOOST_TEST_CASE( & test_locking) );
    test->add( BOOST_TEST_CASE( & test_exclusive) );

	return test;
}
