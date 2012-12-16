
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
#include <boost/fiber/detail/default_scheduler.hpp>

namespace stm = boost::fibers;
namespace this_stm = boost::this_fiber;

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
{ this_stm::yield(); }

void f3()
{
    stm::fiber s( f2);
    BOOST_CHECK( s);
    s.cancel();
    BOOST_CHECK( ! s);
}

void f4()
{
    stm::fiber s( f2);
    BOOST_CHECK( s);
    //BOOST_CHECK( s.is_joinable() );
    BOOST_CHECK( s.join() );
    BOOST_CHECK( ! s);
    //BOOST_CHECK( ! s.is_joinable() );
}

void f5()
{ this_stm::yield_break(); }

void f6( int & i)
{
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

void f7( int t, int & i)
{
    this_stm::sleep( boost::chrono::seconds( t) );
    i = 7;
}

void f8( int t, stm::fiber & s, int & i)
{
    this_stm::sleep( boost::chrono::seconds( t) );
    s.cancel();
    i = 3;
}

void test_move()
{
    {
        stm::fiber s1;
        BOOST_CHECK( s1.empty() );
        stm::fiber s2( f1);
        BOOST_CHECK( ! s2.empty() );
        s1 = boost::move( s2);
        BOOST_CHECK( ! s1.empty() );
        BOOST_CHECK( s2.empty() );
    }

    {
        copyable cp( 3);
        stm::fiber s( cp);
    }

    {
        moveable mv( 7);
        stm::fiber s( boost::move( mv) );
    }
}

void test_id()
{
    stm::fiber s1;
    stm::fiber s2( f1);
    BOOST_CHECK( s1.empty() );
    BOOST_CHECK( ! s2.empty() );

    BOOST_CHECK_EQUAL( stm::fiber::id(), s1.get_id() );
    BOOST_CHECK( stm::fiber::id() != s2.get_id() );

    stm::fiber s3( f1);
    BOOST_CHECK( s2.get_id() != s3.get_id() );

    s1 = boost::move( s2);
    BOOST_CHECK( ! s1.empty() );
    BOOST_CHECK( s2.empty() );

    BOOST_CHECK( stm::fiber::id() != s1.get_id() );
    BOOST_CHECK_EQUAL( stm::fiber::id(), s2.get_id() );
}

void test_detach()
{
    stm::fiber s1( f1);
    BOOST_CHECK( ! s1);
    s1.detach();
    BOOST_CHECK( ! s1);

    stm::fiber s2( f2);
    BOOST_CHECK( s2);
    s2.detach();
    BOOST_CHECK( ! s2);
}

void test_replace()
{
    stm::scheduler::replace(
        new stm::detail::default_scheduler() );
    stm::fiber s1( f1);
    BOOST_CHECK( ! s1);
    stm::fiber s2( f2);
    BOOST_CHECK( s2);
}

void test_complete()
{
    stm::fiber s1( f1);
    BOOST_CHECK( ! s1);
    stm::fiber s2( f2);
    BOOST_CHECK( s2);
}

void test_cancel()
{
    {
        stm::fiber s( f2);
        BOOST_CHECK( s);
        s.cancel();
        BOOST_CHECK( ! s);
    }

    {
        // spawn fiber s
        // s spawns an new fiber s' in its fiber-fn
        // s' yields in its fiber-fn
        // s cancels s' and completes
        stm::fiber s( f3);
        BOOST_CHECK( ! stm::run() );
        BOOST_CHECK( ! s);
        BOOST_CHECK( ! stm::run() );
    }
}

void test_join()
{
    {
        stm::fiber s( f2);
        BOOST_CHECK( s);
        //BOOST_CHECK( s.is_joinable() );
        s.join();
        BOOST_CHECK( ! s);
        //BOOST_CHECK( ! s.is_joinable() );
    }

    {
        stm::fiber s( f2);
        BOOST_CHECK( s);
        //BOOST_CHECK( s.is_joinable() );
        BOOST_CHECK( stm::run() );
        BOOST_CHECK( ! stm::run() );
        BOOST_CHECK( ! s);
        //BOOST_CHECK( ! s.is_joinable() );
    }

    {
        // spawn fiber s
        // s spawns an new fiber s' in its fiber-fn
        // s' yields in its fiber-fn
        // s joins s' and gets suspended (waiting on s') 
        stm::fiber s( f4);
        // run() resumes s' which completes
        BOOST_CHECK( stm::run() );
        // run() resumes s which completes
        BOOST_CHECK( stm::run() );
        BOOST_CHECK( ! s);
        BOOST_CHECK( ! stm::run() );
    }
}

void test_yield_break()
{
    stm::fiber s( f5);
    BOOST_CHECK( ! s);
    BOOST_CHECK( ! stm::run() );
}

void test_yield()
{
    int v1 = 0, v2 = 0;
    BOOST_CHECK_EQUAL( 0, v1);
    BOOST_CHECK_EQUAL( 0, v2);
    stm::fiber s1( boost::bind( f6, boost::ref( v1) ) );
    stm::fiber s2( boost::bind( f6, boost::ref( v2) ) );
    while ( stm::run() );
    BOOST_CHECK( ! s1);
    BOOST_CHECK( ! s2);
    BOOST_CHECK_EQUAL( 8, v1);
    BOOST_CHECK_EQUAL( 8, v2);
}

void test_sleep()
{
    int v1 = 0, v2 = 0;
    BOOST_CHECK_EQUAL( 0, v1);
    BOOST_CHECK_EQUAL( 0, v2);
    stm::fiber s1( boost::bind( f6, boost::ref( v1) ) );
    stm::run();
    BOOST_CHECK( s1);
    stm::fiber s2( boost::bind( f7, 5, boost::ref( v2) ) );
    BOOST_CHECK( s2);
    while ( s1)
        stm::run();
    while ( s2)
        stm::run();
    BOOST_CHECK_EQUAL( 8, v1);
    BOOST_CHECK_EQUAL( 7, v2);
}

void test_sleep_and_cancel()
{
    {
        int v1 = 0, v2 = 0;
        BOOST_CHECK_EQUAL( 0, v1);
        BOOST_CHECK_EQUAL( 0, v2);
        stm::fiber s1( boost::bind( f7, 3, boost::ref( v1) ) );
        stm::fiber s2( boost::bind( f8, 5, boost::ref( s1), boost::ref( v2) ) );
        BOOST_CHECK( s1);
        BOOST_CHECK( s2);
        while ( s1)
            stm::run();
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
        stm::fiber s1( boost::bind( f7, 5, boost::ref( v1) ) );
        stm::fiber s2( boost::bind( f8, 3, boost::ref( s1), boost::ref( v2) ) );
        BOOST_CHECK( s1);
        BOOST_CHECK( s2);
        while ( s2)
            stm::run();
        BOOST_CHECK( ! s1);
        BOOST_CHECK( ! s2);
        BOOST_CHECK_EQUAL( 0, v1);
        BOOST_CHECK_EQUAL( 3, v2);
    }
}

boost::unit_test::test_suite * init_unit_test_suite( int, char* [])
{
    boost::unit_test::test_suite * test =
        BOOST_TEST_SUITE("Boost.Stratified: fiber test suite");

    test->add( BOOST_TEST_CASE( & test_move) );
    test->add( BOOST_TEST_CASE( & test_id) );
    test->add( BOOST_TEST_CASE( & test_detach) );
    test->add( BOOST_TEST_CASE( & test_complete) );
    test->add( BOOST_TEST_CASE( & test_replace) );
    test->add( BOOST_TEST_CASE( & test_cancel) );
    test->add( BOOST_TEST_CASE( & test_join) );
    test->add( BOOST_TEST_CASE( & test_yield_break) );
    test->add( BOOST_TEST_CASE( & test_yield) );
    test->add( BOOST_TEST_CASE( & test_sleep) );
    test->add( BOOST_TEST_CASE( & test_sleep_and_cancel) );

    return test;
}
