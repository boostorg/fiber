
//          Copyright Oliver Kowalke 2013.
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
#include <boost/chrono.hpp>
#include <boost/function.hpp>
#include <boost/ref.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/thread.hpp>
#include <boost/utility.hpp>

#include <boost/fiber/all.hpp>

typedef boost::chrono::nanoseconds  ns;
typedef boost::chrono::milliseconds ms;

int value1 = 0;
int value2 = 0;

template< typename Mtx >
void g( boost::barrier & b, Mtx & m)
{
    b.wait();
    m.lock();
    value1 = 3;
    m.unlock();
}

template< typename Mtx >
void f( boost::barrier & b, Mtx & m)
{
    b.wait();
    m.lock();
    value2 = 7;
    m.unlock();
}

template< typename Mtx >
void fn1( boost::barrier & b, Mtx & m)
{
    boost::fibers::fiber( boost::bind( g< Mtx >, boost::ref( b), boost::ref( m) ) ).join();
}

template< typename Mtx >
void fn2( boost::barrier & b, Mtx & m)
{
    boost::fibers::fiber( boost::bind( f< Mtx >, boost::ref( b), boost::ref( m) ) ).join();
}

void test_mutex()
{
    for ( int i = 0; i < 50; ++i)
    {
        boost::fibers::mutex mtx;
        mtx.lock();
        boost::barrier b( 3);
        boost::thread t1( fn1< boost::fibers::mutex >, boost::ref( b), boost::ref( mtx) );
        boost::thread t2( fn2< boost::fibers::mutex >, boost::ref( b), boost::ref( mtx) );
        b.wait();
        boost::this_thread::sleep_for( ms( 250) );
        mtx.unlock();
        t1.join();
        t2.join();
        BOOST_CHECK( 3 == value1);
        BOOST_CHECK( 7 == value2);
    }
}

void test_recursive_mutex()
{
    for ( int i = 0; i < 50; ++i)
    {
        boost::fibers::recursive_mutex mtx;
        mtx.lock();
        boost::barrier b( 3);
        boost::thread t1( fn1< boost::fibers::recursive_mutex >, boost::ref( b), boost::ref( mtx) );
        boost::thread t2( fn2< boost::fibers::recursive_mutex >, boost::ref( b), boost::ref( mtx) );
        b.wait();
        boost::this_thread::sleep_for( ms( 250) );
        mtx.unlock();
        t1.join();
        t2.join();
        BOOST_CHECK( 3 == value1);
        BOOST_CHECK( 7 == value2);
    }
}

void test_recursive_timed_mutex()
{
    for ( int i = 0; i < 50; ++i)
    {
        boost::fibers::recursive_timed_mutex mtx;
        mtx.lock();
        boost::barrier b( 3);
        boost::thread t1( fn1< boost::fibers::recursive_timed_mutex >, boost::ref( b), boost::ref( mtx) );
        boost::thread t2( fn2< boost::fibers::recursive_timed_mutex >, boost::ref( b), boost::ref( mtx) );
        b.wait();
        boost::this_thread::sleep_for( ms( 250) );
        mtx.unlock();
        t1.join();
        t2.join();
        BOOST_CHECK( 3 == value1);
        BOOST_CHECK( 7 == value2);
    }
}

void test_timed_mutex()
{
    for ( int i = 0; i < 50; ++i)
    {
        boost::fibers::timed_mutex mtx;
        mtx.lock();
        boost::barrier b( 3);
        boost::thread t1( fn1< boost::fibers::timed_mutex >, boost::ref( b), boost::ref( mtx) );
        boost::thread t2( fn2< boost::fibers::timed_mutex >, boost::ref( b), boost::ref( mtx) );
        b.wait();
        boost::this_thread::sleep_for( ms( 250) );
        mtx.unlock();
        t1.join();
        t2.join();
        BOOST_CHECK( 3 == value1);
        BOOST_CHECK( 7 == value2);
    }
}

boost::unit_test::test_suite * init_unit_test_suite( int, char* [])
{
    boost::unit_test::test_suite * test =
        BOOST_TEST_SUITE("Boost.Fiber: multithreaded mutex test suite");

    test->add( BOOST_TEST_CASE( & test_mutex) );
    test->add( BOOST_TEST_CASE( & test_recursive_mutex) );
    test->add( BOOST_TEST_CASE( & test_recursive_timed_mutex) );
    test->add( BOOST_TEST_CASE( & test_timed_mutex) );

	return test;
}
