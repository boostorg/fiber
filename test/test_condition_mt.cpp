
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// This test is based on the tests of Boost.Thread 

#include <chrono>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <map>
#include <stdexcept>
#include <vector>

#include <boost/atomic.hpp>
#include <boost/bind.hpp>
#include <boost/cstdint.hpp>
#include <boost/function.hpp>
#include <boost/ref.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/thread.hpp>
#include <boost/utility.hpp>

#include <boost/fiber/all.hpp>

typedef std::chrono::nanoseconds  ns;
typedef std::chrono::milliseconds ms;

boost::atomic< int > value;

void wait_fn( boost::barrier & b,
              boost::fibers::mutex & mtx,
              boost::fibers::condition & cond,
              bool & flag) {
    b.wait();
	std::unique_lock< boost::fibers::mutex > lk( mtx);
	cond.wait( lk, [&flag](){ return flag; });
	++value;
}

void notify_one_fn( boost::barrier & b,
                    boost::fibers::mutex & mtx,
                    boost::fibers::condition & cond,
                    bool & flag) {
    b.wait();
	std::unique_lock< boost::fibers::mutex > lk( mtx);
    flag = true;
	cond.notify_one();
}

void notify_all_fn( boost::barrier & b,
                    boost::fibers::mutex & mtx,
                    boost::fibers::condition & cond,
                    bool & flag) {
    b.wait();
	std::unique_lock< boost::fibers::mutex > lk( mtx);
    flag = true;
	cond.notify_all();
}

void fn1( boost::barrier & b,
          boost::fibers::mutex & mtx,
          boost::fibers::condition & cond,
          bool & flag) {
    boost::fibers::fiber(
            std::bind(
                wait_fn,
                std::ref( b),
                std::ref( mtx),
                std::ref( cond),
                std::ref( flag) ) ).join();
}

void fn2( boost::barrier & b,
          boost::fibers::mutex & mtx,
          boost::fibers::condition & cond,
          bool & flag) {
	boost::fibers::fiber(
            std::bind(
                notify_one_fn,
                std::ref( b),
                std::ref( mtx),
                std::ref( cond),
                std::ref( flag) ) ).join();
}

void fn3( boost::barrier & b,
          boost::fibers::mutex & mtx,
          boost::fibers::condition & cond,
          bool & flag) {
	boost::fibers::fiber(
            std::bind(
                notify_all_fn,
                std::ref( b),
                std::ref( mtx),
                std::ref( cond),
                std::ref( flag) ) ).join();
}

void test_one_waiter_notify_one() {
    for ( int i = 0; i < 10; ++i) {
        boost::barrier b( 2);

        bool flag = false;
        value = 0;
        boost::fibers::mutex mtx;
        boost::fibers::condition cond;

        BOOST_CHECK( 0 == value);

        boost::thread t1(std::bind( fn1, std::ref( b), std::ref( mtx), std::ref( cond), std::ref( flag) ) );
        boost::thread t2(std::bind( fn2, std::ref( b), std::ref( mtx), std::ref( cond), std::ref( flag) ) );

        BOOST_CHECK( 0 == value);

        t1.join();
        t2.join();

        BOOST_CHECK( 1 == value);
    }
}

void test_two_waiter_notify_all() {
    for ( int i = 0; i < 10; ++i) {
        boost::barrier b( 3);

        bool flag = false;
        value = 0;
        boost::fibers::mutex mtx;
        boost::fibers::condition cond;

        BOOST_CHECK( 0 == value);

        boost::thread t1(std::bind( fn1, std::ref( b), std::ref( mtx), std::ref( cond), std::ref( flag) ) );
        boost::thread t2(std::bind( fn1, std::ref( b), std::ref( mtx), std::ref( cond), std::ref( flag) ) );
        boost::thread t3(std::bind( fn3, std::ref( b), std::ref( mtx), std::ref( cond), std::ref( flag) ) );

        BOOST_CHECK( 0 == value);

        t1.join();
        t2.join();
        t3.join();

        BOOST_CHECK( 2 == value);
    }
}

void test_dummy() {
}

boost::unit_test::test_suite * init_unit_test_suite( int, char* [])
{
    boost::unit_test::test_suite * test =
        BOOST_TEST_SUITE("Boost.Fiber: multithreaded condition test suite");

#if ! defined(BOOST_FIBERS_NO_ATOMICS)
    test->add( BOOST_TEST_CASE( & test_one_waiter_notify_one) );
//    test->add( BOOST_TEST_CASE( & test_two_waiter_notify_all) );
#else
    test->add( BOOST_TEST_CASE( & test_dummy) );
#endif

	return test;
}
