
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <sstream>
#include <string>

#include <boost/assert.hpp>
#include <boost/bind.hpp>
#include <boost/chrono/system_clocks.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/utility.hpp>

#include <boost/fiber/all.hpp>

namespace pt = boost::posix_time;

struct X : private boost::noncopyable
{
    X()
    { BOOST_ASSERT( boost::this_fiber::is_fiberized() ); }

    ~X()
    { BOOST_ASSERT( boost::this_fiber::is_fiberized() ); }
};

void f1( int & i)
{
    X x;
    i = 1;
    boost::this_fiber::yield();
    i = 1;
    boost::this_fiber::yield();
    i = 2;
    boost::this_fiber::yield();
    i = 3;
    boost::this_fiber::yield();
    i = 5;
    boost::this_fiber::yield();
    i = 8;
}

void f2( int t, int & i)
{
    boost::this_fiber::sleep( boost::chrono::seconds( t) );
    i = 7;
}

void f3( int t, int & i)
{
    X x;
    f2( t, i);
}

void test_waitfor_all()
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    int v1 = 0, v2 = 0;
    BOOST_CHECK_EQUAL( 0, v1);
    BOOST_CHECK_EQUAL( 0, v2);
    boost::fibers::fiber s1( boost::bind( f1, boost::ref( v1) ) );
    boost::fibers::fiber s2( boost::bind( f3, 5, boost::ref( v2) ) );
    BOOST_CHECK( s1);
    BOOST_CHECK( s2);
    boost::fibers::waitfor_all( s1, s2);
    BOOST_CHECK( ! s1);
    BOOST_CHECK( ! s2);
    BOOST_CHECK_EQUAL( 8, v1);
    BOOST_CHECK_EQUAL( 7, v2);
}

void test_waitfor_any()
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    int v1 = 0, v2 = 0;
    BOOST_CHECK_EQUAL( 0, v1);
    BOOST_CHECK_EQUAL( 0, v2);
    boost::fibers::fiber s1( boost::bind( f2, 2, boost::ref( v1) ) );
    boost::fibers::fiber s2( boost::bind( f2, 5, boost::ref( v2) ) );
    BOOST_CHECK( s1);
    BOOST_CHECK( s2);
    unsigned int i = boost::fibers::waitfor_any( s1, s2);
    BOOST_CHECK( ! s1);
    BOOST_CHECK( s2);
    BOOST_CHECK_EQUAL( 1, i);
    BOOST_CHECK_EQUAL( 7, v1);
    BOOST_CHECK_EQUAL( 0, v2);
}

void test_waitfor_any_and_cancel()
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    int v1 = 0, v2 = 0;
    BOOST_CHECK_EQUAL( 0, v1);
    BOOST_CHECK_EQUAL( 0, v2);
    boost::fibers::fiber s1( boost::bind( f3, 2, boost::ref( v1) ) );
    boost::fibers::fiber s2( boost::bind( f3, 5, boost::ref( v2) ) );
    BOOST_CHECK( s1);
    BOOST_CHECK( s2);
    unsigned int i = boost::fibers::waitfor_any_and_cancel( s1, s2);
    BOOST_CHECK( ! s1);
    BOOST_CHECK( ! s2);
    BOOST_CHECK_EQUAL( 1, i);
    BOOST_CHECK_EQUAL( 7, v1);
    BOOST_CHECK_EQUAL( 0, v2);
}

boost::unit_test::test_suite * init_unit_test_suite( int, char* [])
{
    boost::unit_test::test_suite * test =
        BOOST_TEST_SUITE("Boost.Fiber: iwaitfor test suite");

    test->add( BOOST_TEST_CASE( & test_waitfor_all) );
    test->add( BOOST_TEST_CASE( & test_waitfor_any) );
    test->add( BOOST_TEST_CASE( & test_waitfor_any_and_cancel) );

    return test;
}
