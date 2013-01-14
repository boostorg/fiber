
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <sstream>
#include <string>

#include <boost/test/unit_test.hpp>
#include <boost/thread.hpp>
#include <boost/utility.hpp>

#include <boost/fiber/all.hpp>

boost::fibers::round_robin * other_ds = 0;
boost::fibers::fiber * other_f = 0;

void lazy_generate( boost::barrier * b, int * value)
{
    * value = 1;
    boost::this_fiber::yield();

    if ( b) b->wait();

    * value = 2;
    boost::this_fiber::yield();
    boost::this_fiber::interruption_point();
    * value = 3;
    boost::this_fiber::yield();
    boost::this_fiber::interruption_point();
    * value = 4;
    boost::this_fiber::yield();
    boost::this_fiber::interruption_point();
    * value = 5;
    boost::this_fiber::yield();
    boost::this_fiber::interruption_point();
    * value = 6;
}

void join_fiber( boost::barrier * b, int * value, bool * interrupted)
{
    b->wait();
    try
    {
        other_f->join();
        * value = 7;
    }
    catch ( boost::fibers::fiber_interrupted const&)
    { * interrupted = true; }
}

void interrupt_join_fiber( boost::barrier * b, int * value, bool * interrupted)
{
    b->wait();
    other_f->interrupt();
    try
    {
        other_f->join();
        * value = 7;
    }
    catch ( boost::fibers::fiber_interrupted const&)
    { * interrupted = true; }
}



void fn_running( boost::barrier * b, int * value)
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);
    other_ds = & ds;

    other_f = new boost::fibers::fiber(
            boost::bind( lazy_generate, b, value) );
    // other_f will joined by another fiber
}

void fn_terminated( boost::barrier * b, int * value)
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);
    other_ds = & ds;

    other_f = new boost::fibers::fiber(
            boost::bind( lazy_generate, ( boost::barrier *) 0, value) );
    other_f->join();
    b->wait();
}

void fn_interrupt_from_same_thread( boost::barrier * b, int * value, bool * interrupted)
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    other_f = new boost::fibers::fiber(
            boost::bind( lazy_generate, b, value) );
    other_f->interrupt();
    try
    { other_f->join(); }
    catch ( boost::fibers::fiber_interrupted const&)
    { * interrupted = true; }
}

void fn_join( boost::barrier * b, int * value, bool * interrupted)
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    join_fiber( b, value, interrupted);
}

void fn_join_in_fiber( boost::barrier * b, int * value, bool * interrupted)
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    boost::fibers::fiber(
        boost::bind( join_fiber, b, value, interrupted) ).join();
}

void fn_join_interrupt( boost::barrier * b, int * value, bool * interrupted)
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    boost::fibers::fiber(
        boost::bind( interrupt_join_fiber, b, value, interrupted) ).join();
}

void test_join_runing()
{
    int value1 = 0;
    int value2 = 0;
    bool interrupted = false;

    boost::barrier b( 2);
    boost::thread t1( boost::bind( fn_running, &b, &value1) );
    boost::thread t2( boost::bind( fn_join, &b, &value2, &interrupted) );

    t1.join();
    t2.join();

    BOOST_CHECK_EQUAL( 6, value1);
    BOOST_CHECK_EQUAL( 7, value2);
    BOOST_CHECK( ! interrupted);
    delete other_f;
}

void test_join_in_fiber_runing()
{
    int value1 = 0;
    int value2 = 0;
    bool interrupted = false;

    boost::barrier b( 2);
    boost::thread t1( boost::bind( fn_running, &b, &value1) );
    boost::thread t2( boost::bind( fn_join_in_fiber, &b, &value2, &interrupted) );

    t1.join();
    t2.join();

    BOOST_CHECK_EQUAL( 6, value1);
    BOOST_CHECK_EQUAL( 7, value2);
    BOOST_CHECK( ! interrupted);
    delete other_f;
}

void test_join_terminated()
{
    int value1 = 0;
    int value2 = 0;
    bool interrupted = false;

    boost::barrier b( 2);
    boost::thread t1( boost::bind( fn_terminated, &b, &value1) );
    boost::thread t2( boost::bind( fn_join, &b, &value2, &interrupted) );

    t1.join();
    t2.join();

    BOOST_CHECK_EQUAL( 6, value1);
    BOOST_CHECK_EQUAL( 7, value2);
    BOOST_CHECK( ! interrupted);
    delete other_f;
}

void test_join_in_fiber_terminated()
{
    int value1 = 0;
    int value2 = 0;
    bool interrupted = false;

    boost::barrier b( 2);
    boost::thread t1( boost::bind( fn_terminated, &b, &value1) );
    boost::thread t2( boost::bind( fn_join_in_fiber, &b, &value2, &interrupted) );

    t1.join();
    t2.join();

    BOOST_CHECK_EQUAL( 6, value1);
    BOOST_CHECK_EQUAL( 7, value2);
    BOOST_CHECK( ! interrupted);
    delete other_f;
}

void test_join_interrupted_inside()
{
    for ( int i = 0; i < 1; ++i) {
    int value1 = 0;
    int value2 = 0;
    bool interrupted1 = false, interrupted2 = false;

    boost::barrier b( 2);
    boost::thread t1( boost::bind( fn_interrupt_from_same_thread, &b, &value1, &interrupted1) );
    boost::thread t2( boost::bind( fn_join, &b, &value2, &interrupted2) );

    t1.join();
    t2.join();

    BOOST_CHECK( interrupted1);
    BOOST_CHECK( interrupted2);
    BOOST_CHECK_EQUAL( 0, value2);
    delete other_f;
    }
}

boost::unit_test::test_suite * init_unit_test_suite( int, char* [])
{
    boost::unit_test::test_suite * test =
        BOOST_TEST_SUITE("Boost.Fiber: round_robin test suite");

#if 0
    test->add( BOOST_TEST_CASE( & test_join_runing) );
#endif
    test->add( BOOST_TEST_CASE( & test_join_in_fiber_runing) );
#if 0
    test->add( BOOST_TEST_CASE( & test_join_terminated) );
    test->add( BOOST_TEST_CASE( & test_join_in_fiber_runing) );
    test->add( BOOST_TEST_CASE( & test_join_in_fiber_terminated) );
    test->add( BOOST_TEST_CASE( & test_join_interrupted_inside) );
#endif
    return test;
}
