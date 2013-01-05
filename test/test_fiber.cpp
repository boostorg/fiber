
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

class copyable
{
private:
    int i_;

public:
    copyable( int i) :
        i_( i)
    {}

    int operator()()
    { return i_; }
};

class moveable
{
private:
    bool    state_;
    int     i_;

    BOOST_MOVABLE_BUT_NOT_COPYABLE( moveable);

public:
    moveable() :
        state_( false), i_( 0)
    {}

    moveable( int i) :
        state_( false), i_( i)
    {}

    moveable( BOOST_RV_REF( moveable) other) :
        state_( false), i_( 0)
    {
        std::swap( state_, other.state_);
        std::swap( i_, other.i_);
    }

    moveable & operator=( BOOST_RV_REF( moveable) other)
    {
        if ( this == & other) return * this;
        moveable tmp( boost::move( other) );
        std::swap( state_, tmp.state_);
        std::swap( i_, tmp.i_);
        return * this;
    }

    int operator()()
    { return i_; }
};

void f1() {}

void f2()
{
    boost::this_fiber::yield();
}

void f3()
{
    boost::fibers::fiber s( f2);
    BOOST_CHECK( s);
    s.cancel();
    BOOST_CHECK( ! s);
}

void f4()
{
    boost::fibers::fiber s( f2);
    std::cout << s.get_id() << "\n";
    BOOST_CHECK( s);
    BOOST_CHECK( s.joinable() );
    s.join();
    BOOST_CHECK( ! s);
    BOOST_CHECK( ! s.joinable() );
}

void f5()
{ boost::this_fiber::yield_break(); }

void f6( int & i)
{
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

void f7( int t, int & i)
{
    boost::this_fiber::sleep( boost::chrono::seconds( t) );
    i = 7;
}

void f8( int t, boost::fibers::fiber & s, int & i)
{
    boost::this_fiber::sleep( boost::chrono::seconds( t) );
    s.cancel();
    i = 3;
}

void test_move()
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    {
        boost::fibers::fiber s1;
        BOOST_CHECK( s1.empty() );
        boost::fibers::fiber s2( f1);
        BOOST_CHECK( ! s2.empty() );
        s1 = boost::move( s2);
        BOOST_CHECK( ! s1.empty() );
        BOOST_CHECK( s2.empty() );
    }

    {
        copyable cp( 3);
        boost::fibers::fiber s( cp);
    }

    {
        moveable mv( 7);
        boost::fibers::fiber s( boost::move( mv) );
    }
}

void test_priority()
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    boost::fibers::fiber f( f1);
    BOOST_CHECK_EQUAL( 0, f.priority() );
    f.priority( 7);
    BOOST_CHECK_EQUAL( 7, f.priority() );
}

void test_id()
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    boost::fibers::fiber s1;
    boost::fibers::fiber s2( f1);
    BOOST_CHECK( s1.empty() );
    BOOST_CHECK( ! s2.empty() );

    BOOST_CHECK_EQUAL( boost::fibers::fiber::id(), s1.get_id() );
    BOOST_CHECK( boost::fibers::fiber::id() != s2.get_id() );

    boost::fibers::fiber s3( f1);
    BOOST_CHECK( s2.get_id() != s3.get_id() );

    s1 = boost::move( s2);
    BOOST_CHECK( ! s1.empty() );
    BOOST_CHECK( s2.empty() );

    BOOST_CHECK( boost::fibers::fiber::id() != s1.get_id() );
    BOOST_CHECK_EQUAL( boost::fibers::fiber::id(), s2.get_id() );
}

void test_detach()
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    {
        boost::fibers::fiber s1( f1);
        BOOST_CHECK( ! s1);
        s1.detach();
        BOOST_CHECK( ! s1);
    }

    {
        boost::fibers::fiber s2( f2);
        BOOST_CHECK( s2);
        s2.detach();
        BOOST_CHECK( ! s2);
    }
}

void test_replace()
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    boost::fibers::scheduling_algorithm(
        new boost::fibers::round_robin() );
    boost::fibers::fiber s1( f1);
    BOOST_CHECK( ! s1);
    boost::fibers::fiber s2( f2);
    BOOST_CHECK( s2);
    boost::fibers::run();
}

void test_complete()
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    boost::fibers::fiber s1( f1);
    BOOST_CHECK( ! s1);
    boost::fibers::fiber s2( f2);
    BOOST_CHECK( s2);
    boost::fibers::run();
}

void test_cancel()
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    {
        boost::fibers::fiber s( f2);
        BOOST_CHECK( s);
        s.cancel();
        BOOST_CHECK( ! s);
    }

    {
        // spawn fiber s
        // s spawns an new fiber s' in its fiber-fn
        // s' yields in its fiber-fn
        // s cancels s' and completes
        boost::fibers::fiber s( f3);
        BOOST_CHECK( ! boost::fibers::run() );
        BOOST_CHECK( ! s);
        BOOST_CHECK( ! boost::fibers::run() );
    }
}

void test_join_in_thread()
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    boost::fibers::fiber s( f2);
    BOOST_CHECK( s);
    BOOST_CHECK( s.joinable() );
    s.join();
    BOOST_CHECK( ! s);
    BOOST_CHECK( ! s.joinable() );
}

void test_join_and_run()
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    boost::fibers::fiber s( f2);
    BOOST_CHECK( s);
    BOOST_CHECK( s.joinable() );
    BOOST_CHECK( boost::fibers::run() );
    BOOST_CHECK( ! boost::fibers::run() );
    BOOST_CHECK( ! s);
    BOOST_CHECK( ! s.joinable() );
}

void test_join_in_fiber()
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    // spawn fiber s
    // s spawns an new fiber s' in its fiber-fn
    // s' yields in its fiber-fn
    // s joins s' and gets suspended (waiting on s')
    boost::fibers::fiber s( f4);
    std::cout << s.get_id() << "\n";
    // run() resumes s + s' which completes
    while ( s)
        boost::fibers::run();
    BOOST_CHECK( ! s);
    BOOST_CHECK( ! boost::fibers::run() );
}

void test_yield_break()
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    boost::fibers::fiber s( f5);
    BOOST_CHECK( ! s);
    BOOST_CHECK( ! boost::fibers::run() );
}

void test_yield()
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    int v1 = 0, v2 = 0;
    BOOST_CHECK_EQUAL( 0, v1);
    BOOST_CHECK_EQUAL( 0, v2);
    boost::fibers::fiber s1( boost::bind( f6, boost::ref( v1) ) );
    boost::fibers::fiber s2( boost::bind( f6, boost::ref( v2) ) );
    while ( boost::fibers::run() );
    BOOST_CHECK( ! s1);
    BOOST_CHECK( ! s2);
    BOOST_CHECK_EQUAL( 8, v1);
    BOOST_CHECK_EQUAL( 8, v2);
}

void test_sleep()
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    int v1 = 0, v2 = 0;
    BOOST_CHECK_EQUAL( 0, v1);
    BOOST_CHECK_EQUAL( 0, v2);
    boost::fibers::fiber s1( boost::bind( f6, boost::ref( v1) ) );
    boost::fibers::run();
    BOOST_CHECK( s1);
    boost::fibers::fiber s2( boost::bind( f7, 5, boost::ref( v2) ) );
    BOOST_CHECK( s2);
    while ( s1)
        boost::fibers::run();
    while ( s2)
        boost::fibers::run();
    BOOST_CHECK_EQUAL( 8, v1);
    BOOST_CHECK_EQUAL( 7, v2);
}

void test_sleep_and_cancel()
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    {
        int v1 = 0, v2 = 0;
        BOOST_CHECK_EQUAL( 0, v1);
        BOOST_CHECK_EQUAL( 0, v2);
        boost::fibers::fiber s1( boost::bind( f7, 3, boost::ref( v1) ) );
        boost::fibers::fiber s2( boost::bind( f8, 5, boost::ref( s1), boost::ref( v2) ) );
        BOOST_CHECK( s1);
        BOOST_CHECK( s2);
        while ( s1)
            boost::fibers::run();
        BOOST_CHECK_EQUAL( 7, v1);
        BOOST_CHECK_EQUAL( 0, v2);
        BOOST_CHECK( s2);
        s2.cancel();
        BOOST_CHECK( ! s2);
        BOOST_CHECK_EQUAL( 7, v1);
        BOOST_CHECK_EQUAL( 0, v2);
    }
    {
        int v1 = 0, v2 = 0;
        BOOST_CHECK_EQUAL( 0, v1);
        BOOST_CHECK_EQUAL( 0, v2);
        boost::fibers::fiber s1( boost::bind( f7, 5, boost::ref( v1) ) );
        boost::fibers::fiber s2( boost::bind( f8, 3, boost::ref( s1), boost::ref( v2) ) );
        BOOST_CHECK( s1);
        BOOST_CHECK( s2);
        while ( s2)
            boost::fibers::run();
        BOOST_CHECK( ! s1);
        BOOST_CHECK( ! s2);
        BOOST_CHECK_EQUAL( 0, v1);
        BOOST_CHECK_EQUAL( 3, v2);
    }
}

boost::unit_test::test_suite * init_unit_test_suite( int, char* [])
{
    boost::unit_test::test_suite * test =
        BOOST_TEST_SUITE("Boost.Fiber: fiber test suite");
    test->add( BOOST_TEST_CASE( & test_move) );
    test->add( BOOST_TEST_CASE( & test_id) );
    test->add( BOOST_TEST_CASE( & test_priority) );
    test->add( BOOST_TEST_CASE( & test_detach) );
    test->add( BOOST_TEST_CASE( & test_complete) );
    test->add( BOOST_TEST_CASE( & test_replace) );
    //test->add( BOOST_TEST_CASE( & test_cancel) );
    test->add( BOOST_TEST_CASE( & test_join_in_thread) );
    test->add( BOOST_TEST_CASE( & test_join_and_run) );
    test->add( BOOST_TEST_CASE( & test_join_in_fiber) );
    test->add( BOOST_TEST_CASE( & test_yield_break) );
    test->add( BOOST_TEST_CASE( & test_yield) );
    //test->add( BOOST_TEST_CASE( & test_sleep) );
    //test->add( BOOST_TEST_CASE( & test_sleep_and_cancel) );
    return test;
}
