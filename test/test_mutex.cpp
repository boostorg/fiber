
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
    test_lock< boost::fibers::mutex >()();
}

int value1 = 0;
int value2 = 0;

void fn1( boost::fibers::mutex & mtx)
{
	boost::fibers::mutex::scoped_lock lk( mtx);
	++value1;
	for ( int i = 0; i < 3; ++i)
		boost::this_fiber::yield();
}

void fn2( boost::fibers::mutex & mtx)
{
	++value2;
	boost::fibers::mutex::scoped_lock lk( mtx);
	++value2;
}

void test_locking()
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    boost::fibers::fiber s( & do_test_mutex);
    s.join();
    BOOST_ASSERT( ! s);
}

void test_exclusive()
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    value1 = 0;
    value2 = 0;
	BOOST_CHECK_EQUAL( 0, value1);
	BOOST_CHECK_EQUAL( 0, value2);

	boost::fibers::mutex mtx;
    boost::fibers::fiber s1(
		boost::bind( & fn1, boost::ref( mtx) ) );
    boost::fibers::fiber s2(
		boost::bind( & fn2, boost::ref( mtx) ) );
    BOOST_ASSERT( s1);
    BOOST_ASSERT( s2);
	BOOST_CHECK_EQUAL( 1, value1);
	BOOST_CHECK_EQUAL( 1, value2);

    s1.join();
    s2.join();
	BOOST_CHECK_EQUAL( 1, value1);
	BOOST_CHECK_EQUAL( 2, value2);
}

boost::unit_test::test_suite * init_unit_test_suite( int, char* [])
{
    boost::unit_test::test_suite * test =
        BOOST_TEST_SUITE("Boost.Fiber: mutex test suite");

    test->add( BOOST_TEST_CASE( & test_locking) );
    test->add( BOOST_TEST_CASE( & test_exclusive) );

	return test;
}
