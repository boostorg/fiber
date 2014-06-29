
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// This test is based on the tests of Boost.Thread 

#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <map>
#include <stdexcept>
#include <vector>

#include <boost/atomic.hpp>
#include <boost/bind.hpp>
#include <boost/chrono/system_clocks.hpp>
#include <boost/cstdint.hpp>
#include <boost/function.hpp>
#include <boost/ref.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/thread.hpp>
#include <boost/utility.hpp>

#include <boost/fiber/all.hpp>

typedef boost::chrono::nanoseconds  ns;
typedef boost::chrono::milliseconds ms;

boost::atomic< int > value;

void notify_one_fn( boost::barrier & b, boost::fibers::condition & cond)
{
    b.wait();
    boost::this_thread::sleep_for( ms( 250) );
	cond.notify_one();
}

void notify_all_fn( boost::barrier & b, boost::fibers::condition & cond)
{
    b.wait();
    boost::this_thread::sleep_for( ms( 250) );
	cond.notify_all();
}

void wait_fn(
    boost::barrier & b,
	boost::fibers::mutex & mtx,
	boost::fibers::condition & cond)
{
    b.wait();
	boost::unique_lock< boost::fibers::mutex > lk( mtx);
	cond.wait( lk);
	++value;
}

void fn1( boost::barrier & b, boost::fibers::mutex & mtx, boost::fibers::condition & cond)
{
    boost::fibers::fiber(
            boost::bind(
                wait_fn,
                boost::ref( b),
                boost::ref( mtx),
                boost::ref( cond) ) ).join();
}

void fn2( boost::barrier & b, boost::fibers::condition & cond)
{
	boost::fibers::fiber(
            boost::bind(
                notify_one_fn,
                boost::ref( b),
                boost::ref( cond) ) ).join();
}

void fn3( boost::barrier & b, boost::fibers::condition & cond)
{
	boost::fibers::fiber(
            boost::bind(
                notify_all_fn,
                boost::ref( b),
                boost::ref( cond) ) ).join();
}

void test_one_waiter_notify_one()
{
    boost::barrier b( 2);

	value = 0;
	boost::fibers::mutex mtx;
	boost::fibers::condition cond;

	BOOST_CHECK( 0 == value);

    boost::thread t1(boost::bind( fn1, boost::ref( b), boost::ref( mtx), boost::ref( cond) ) );
    boost::thread t2(boost::bind( fn2, boost::ref( b), boost::ref( cond) ) );

	BOOST_CHECK( 0 == value);

    t1.join();
    t2.join();

	BOOST_CHECK( 1 == value);
}

void test_two_waiter_notify_all()
{
    boost::barrier b( 3);

	value = 0;
	boost::fibers::mutex mtx;
	boost::fibers::condition cond;

	BOOST_CHECK( 0 == value);

    boost::thread t1(boost::bind( fn1, boost::ref( b), boost::ref( mtx), boost::ref( cond) ) );
    boost::thread t2(boost::bind( fn1, boost::ref( b), boost::ref( mtx), boost::ref( cond) ) );
    boost::thread t3(boost::bind( fn3, boost::ref( b), boost::ref( cond) ) );

	BOOST_CHECK( 0 == value);

    t1.join();
    t2.join();
    t3.join();

	BOOST_CHECK( 2 == value);
}

boost::unit_test::test_suite * init_unit_test_suite( int, char* [])
{
    boost::unit_test::test_suite * test =
        BOOST_TEST_SUITE("Boost.Fiber: multithreaded condition test suite");

    test->add( BOOST_TEST_CASE( & test_one_waiter_notify_one) );
    test->add( BOOST_TEST_CASE( & test_two_waiter_notify_all) );

	return test;
}
