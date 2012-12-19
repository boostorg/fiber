
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
namespace stm = boost::fibers;
namespace this_stm = boost::this_fiber;

struct X : private boost::noncopyable
{
    X()
    { BOOST_ASSERT( this_stm::is_fiberized() ); }

    ~X()
    { BOOST_ASSERT( this_stm::is_fiberized() ); }
};

void f1( int & i)
{
    X x;
    i = 1;
    this_stm::yield();
    i = 1;
    this_stm::yield();
    i = 2;
    this_stm::yield();
    i = 3;
    this_stm::yield();
    i = 5;
    this_stm::yield();
    i = 8;
}

void f2( int t, int & i)
{
    this_stm::sleep( boost::chrono::seconds( t) );
    i = 7;
}

void f3( int t, int & i)
{
    X x;
    f2( t, i);
}

void test_waitfor_all()
{
    stm::round_robin ds;
    stm::scheduling_algorithm( & ds);

    int v1 = 0, v2 = 0;
    BOOST_CHECK_EQUAL( 0, v1);
    BOOST_CHECK_EQUAL( 0, v2);
    stm::fiber s1( boost::bind( f1, boost::ref( v1) ) );
    stm::fiber s2( boost::bind( f3, 5, boost::ref( v2) ) );
    BOOST_CHECK( s1);
    BOOST_CHECK( s2);
    stm::waitfor_all( s1, s2);
    BOOST_CHECK( ! s1);
    BOOST_CHECK( ! s2);
    BOOST_CHECK_EQUAL( 8, v1);
    BOOST_CHECK_EQUAL( 7, v2);
}

void test_waitfor_any()
{
    stm::round_robin ds;
    stm::scheduling_algorithm( & ds);

    int v1 = 0, v2 = 0;
    BOOST_CHECK_EQUAL( 0, v1);
    BOOST_CHECK_EQUAL( 0, v2);
    stm::fiber s1( boost::bind( f2, 2, boost::ref( v1) ) );
    stm::fiber s2( boost::bind( f2, 5, boost::ref( v2) ) );
    BOOST_CHECK( s1);
    BOOST_CHECK( s2);
    unsigned int i = stm::waitfor_any( s1, s2);
    BOOST_CHECK( ! s1);
    BOOST_CHECK( s2);
    BOOST_CHECK_EQUAL( 1, i);
    BOOST_CHECK_EQUAL( 7, v1);
    BOOST_CHECK_EQUAL( 0, v2);
}

void test_waitfor_any_and_cancel()
{
    stm::round_robin ds;
    stm::scheduling_algorithm( & ds);

    int v1 = 0, v2 = 0;
    BOOST_CHECK_EQUAL( 0, v1);
    BOOST_CHECK_EQUAL( 0, v2);
    stm::fiber s1( boost::bind( f3, 2, boost::ref( v1) ) );
    stm::fiber s2( boost::bind( f3, 5, boost::ref( v2) ) );
    BOOST_CHECK( s1);
    BOOST_CHECK( s2);
    unsigned int i = stm::waitfor_any_and_cancel( s1, s2);
    BOOST_CHECK( ! s1);
    BOOST_CHECK( ! s2);
    BOOST_CHECK_EQUAL( 1, i);
    BOOST_CHECK_EQUAL( 7, v1);
    BOOST_CHECK_EQUAL( 0, v2);
}

boost::unit_test::test_suite * init_unit_test_suite( int, char* [])
{
    boost::unit_test::test_suite * test =
        BOOST_TEST_SUITE("Boost.Stratified: iwaitfor test suite");

    test->add( BOOST_TEST_CASE( & test_waitfor_all) );
    test->add( BOOST_TEST_CASE( & test_waitfor_any) );
    test->add( BOOST_TEST_CASE( & test_waitfor_any_and_cancel) );

    return test;
}
