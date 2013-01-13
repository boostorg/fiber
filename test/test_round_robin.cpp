
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

int value1 = 0;
int value2 = 0;

boost::fibers::round_robin * other_ds = 0;
boost::fibers::fiber * other_f = 0;

void lazy_generate_running( boost::barrier & b)
{
    value1 = 1;
    boost::this_fiber::yield();
    b.wait();
    value1 = 2;
    boost::this_fiber::yield();
    boost::this_fiber::interruption_point();
    value1 = 3;
    boost::this_fiber::yield();
    boost::this_fiber::interruption_point();
    value1 = 4;
    boost::this_fiber::yield();
    boost::this_fiber::interruption_point();
    value1 = 5;
    boost::this_fiber::yield();
    boost::this_fiber::interruption_point();
    value1 = 6;
}

void lazy_generate()
{
    value1 = 1;
    boost::this_fiber::yield();
    value1 = 2;
    boost::this_fiber::yield();
    value1 = 3;
    boost::this_fiber::yield();
    value1 = 4;
    boost::this_fiber::yield();
    value1 = 5;
    boost::this_fiber::yield();
    value1 = 6;
}

void join_other( boost::barrier & b)
{
    fprintf(stderr, "1\n");
    b.wait();
    fprintf(stderr, "2\n");
    other_f->join();
    fprintf(stderr, "3\n");
    BOOST_CHECK_EQUAL( 6, value1);
    value2 = 7;
}

void join_interrupt_other( boost::barrier & b)
{
    b.wait();
    other_f->interrupt();
    other_f->join();
    BOOST_CHECK_EQUAL( 6, value1);
    value2 = 7;
}

void fn_running( boost::barrier & b)
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);
    other_ds = & ds;

    other_f = new boost::fibers::fiber(
            boost::bind( lazy_generate_running, boost::ref( b) ) );
    other_f->join();
}

void fn_terminated( boost::barrier & b)
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);
    other_ds = & ds;

    other_f = new boost::fibers::fiber( lazy_generate);
    other_f->join();
    b.wait();
}

void fn_interrupted_inside( boost::barrier & b)
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);
    other_ds = & ds;

    bool interrupted = false;
    other_f = new boost::fibers::fiber(
            boost::bind( lazy_generate_running, boost::ref( b) ) );
    fprintf( stderr, "other_f->interrupt()\n");
    other_f->interrupt();
    fprintf( stderr, "other_f->join()\n");
    try
    { other_f->join(); }
    catch ( boost::fibers::fiber_interrupted const&)
    { interrupted = true; }
    BOOST_CHECK( interrupted);
    fprintf( stderr, "fn_interrupted_inside() finished\n");
}

void fn_join( boost::barrier & b)
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    boost::fibers::fiber(
        boost::bind( join_other, boost::ref( b) ) ).join();
}

void fn_join_interrupt( boost::barrier & b)
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    boost::fibers::fiber(
        boost::bind( join_interrupt_other, boost::ref( b) ) ).join();
}

void test_join_runing()
{
    BOOST_CHECK_EQUAL( 0, value1);
    BOOST_CHECK_EQUAL( 0, value2);

    boost::barrier b( 2);
    boost::thread t1( boost::bind( fn_running, boost::ref( b) ) );
    boost::thread t2( boost::bind( fn_join, boost::ref( b) ) );

    t1.join();
    t2.join();

    BOOST_CHECK_EQUAL( 6, value1);
    BOOST_CHECK_EQUAL( 7, value2);
    delete other_f;
}

void test_join_terminated()
{
    value1 = 0;
    value2 = 0;
    BOOST_CHECK_EQUAL( 0, value1);
    BOOST_CHECK_EQUAL( 0, value2);

    boost::barrier b( 2);
    boost::thread t1( boost::bind( fn_terminated, boost::ref( b) ) );
    boost::thread t2( boost::bind( fn_join, boost::ref( b) ) );

    t1.join();
    t2.join();

    BOOST_CHECK_EQUAL( 6, value1);
    BOOST_CHECK_EQUAL( 7, value2);
    delete other_f;
}

void test_join_interrupted_inside()
{
    for ( int i = 0; i < 1000; ++i) {
    value1 = 0;
    value2 = 0;
    BOOST_CHECK_EQUAL( 0, value1);
    BOOST_CHECK_EQUAL( 0, value2);

    boost::barrier b( 2);
    boost::thread t1( boost::bind( fn_interrupted_inside, boost::ref( b) ) );
    boost::thread t2( boost::bind( fn_join, boost::ref( b) ) );

    t1.join();
    fprintf(stderr, "t1.joined\n");
    t2.join();

    BOOST_CHECK_EQUAL( 6, value1);
    BOOST_CHECK_EQUAL( 7, value2);
    delete other_f;
    }
}

void test_join_interrupted_other()
{
    for ( int i = 0; i < 1000; ++i) {
    value1 = 0;
    value2 = 0;
    BOOST_CHECK_EQUAL( 0, value1);
    BOOST_CHECK_EQUAL( 0, value2);

    boost::barrier b( 2);
    boost::thread t1( boost::bind( fn_running, boost::ref( b) ) );
    boost::thread t2( boost::bind( fn_join_interrupt, boost::ref( b) ) );

    t1.join();
    t2.join();

    BOOST_CHECK_EQUAL( 6, value1);
    BOOST_CHECK_EQUAL( 7, value2);
    delete other_f;
    }
}

boost::unit_test::test_suite * init_unit_test_suite( int, char* [])
{
    boost::unit_test::test_suite * test =
        BOOST_TEST_SUITE("Boost.Fiber: round_robin test suite");
#if 0
    test->add( BOOST_TEST_CASE( & test_join_runing) );
    test->add( BOOST_TEST_CASE( & test_join_terminated) );
#endif
    test->add( BOOST_TEST_CASE( & test_join_interrupted_inside) );
    //test->add( BOOST_TEST_CASE( & test_join_interrupted_other) );

    return test;
}
