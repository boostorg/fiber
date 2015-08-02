
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <chrono>
#include <sstream>
#include <string>

#include <boost/assert.hpp>
#include <boost/test/unit_test.hpp>

#include <boost/fiber/all.hpp>

struct moveable {
    bool    state;
    int     value;

    moveable() :
        state( false),
        value( -1)
    {}

    moveable( int v) :
        state( true),
        value( v)
    {}

    moveable( moveable && other) :
        state( other.state),
        value( other.value)
    {
        other.state = false;
        other.value = -1;
    }

    moveable & operator=( moveable && other)
    {
        if ( this == & other) return * this;
        state = other.state;
        other.state = false;
        value = other.value;
        other.value = -1;
        return * this;
    }
};

void test_push()
{
    boost::fibers::bounded_channel< int > c( 10);
    BOOST_CHECK( c.is_empty() );
    BOOST_CHECK( boost::fibers::channel_op_status::success == c.push( 1) );
    BOOST_CHECK( ! c.is_empty() );
}

void test_push_closed()
{
    boost::fibers::bounded_channel< int > c( 10);
    BOOST_CHECK( ! c.is_closed() );
    c.close();
    BOOST_CHECK( c.is_closed() );
    BOOST_CHECK( boost::fibers::channel_op_status::closed == c.push( 1) );
}

void test_pop()
{
    boost::fibers::bounded_channel< int > c( 10);
    BOOST_CHECK( c.is_empty() );
    int v1 = 2, v2 = 0;
    BOOST_CHECK( boost::fibers::channel_op_status::success == c.push( v1) );
    BOOST_CHECK( ! c.is_empty() );
    BOOST_CHECK( boost::fibers::channel_op_status::success == c.pop( v2) );
    BOOST_CHECK( c.is_empty() );
    BOOST_CHECK_EQUAL( v1, v2);
}

void test_pop_closed()
{
    boost::fibers::bounded_channel< int > c( 10);
    BOOST_CHECK( c.is_empty() );
    int v1 = 2, v2 = 0;
    BOOST_CHECK( boost::fibers::channel_op_status::success == c.push( v1) );
    BOOST_CHECK( ! c.is_empty() );
    BOOST_CHECK( ! c.is_closed() );
    c.close();
    BOOST_CHECK( c.is_closed() );
    BOOST_CHECK( boost::fibers::channel_op_status::success == c.pop( v2) );
    BOOST_CHECK( c.is_empty() );
    BOOST_CHECK_EQUAL( v1, v2);
    BOOST_CHECK( boost::fibers::channel_op_status::closed == c.pop( v2) );
}

void test_pop_success()
{
    boost::fibers::bounded_channel< int > c( 10);
    BOOST_CHECK( c.is_empty() );
    int v1 = 2, v2 = 0;
    boost::fibers::fiber f1([&c,&v2](){
        BOOST_CHECK( boost::fibers::channel_op_status::success == c.pop( v2) );
        BOOST_CHECK( c.is_empty() );
    });
    boost::fibers::fiber f2([&c,v1](){
        BOOST_CHECK( boost::fibers::channel_op_status::success == c.push( v1) );
        BOOST_CHECK( ! c.is_empty() );
    });
    f1.join();
    BOOST_CHECK( c.is_empty() );
    f2.join();
    BOOST_CHECK( c.is_empty() );
    BOOST_CHECK_EQUAL( v1, v2);
}

void test_value_pop()
{
    boost::fibers::bounded_channel< int > c( 10);
    BOOST_CHECK( c.is_empty() );
    int v1 = 2, v2 = 0;
    BOOST_CHECK( boost::fibers::channel_op_status::success == c.push( v1) );
    BOOST_CHECK( ! c.is_empty() );
    v2 = c.value_pop();
    BOOST_CHECK( c.is_empty() );
    BOOST_CHECK_EQUAL( v1, v2);
}

void test_value_pop_closed()
{
    boost::fibers::bounded_channel< int > c( 10);
    BOOST_CHECK( c.is_empty() );
    int v1 = 2, v2 = 0;
    BOOST_CHECK( boost::fibers::channel_op_status::success == c.push( v1) );
    BOOST_CHECK( ! c.is_empty() );
    BOOST_CHECK( ! c.is_closed() );
    c.close();
    BOOST_CHECK( c.is_closed() );
    v2 = c.value_pop();
    BOOST_CHECK( c.is_empty() );
    BOOST_CHECK_EQUAL( v1, v2);
    bool thrown = false;
    try {
        c.value_pop();
    } catch ( boost::fibers::fiber_exception const&) {
        thrown = true;
    }
    BOOST_CHECK( thrown);
}

void test_value_pop_success()
{
    boost::fibers::bounded_channel< int > c( 10);
    BOOST_CHECK( c.is_empty() );
    int v1 = 2, v2 = 0;
    boost::fibers::fiber f1([&c,&v2](){
        v2 = c.value_pop();
        BOOST_CHECK( c.is_empty() );
    });
    boost::fibers::fiber f2([&c,v1](){
        BOOST_CHECK( boost::fibers::channel_op_status::success == c.push( v1) );
        BOOST_CHECK( ! c.is_empty() );
    });
    f1.join();
    BOOST_CHECK( c.is_empty() );
    f2.join();
    BOOST_CHECK( c.is_empty() );
    BOOST_CHECK_EQUAL( v1, v2);
}

void test_try_pop()
{
    boost::fibers::bounded_channel< int > c( 10);
    BOOST_CHECK( c.is_empty() );
    int v1 = 2, v2 = 0;
    BOOST_CHECK( boost::fibers::channel_op_status::success == c.push( v1) );
    BOOST_CHECK( ! c.is_empty() );
    BOOST_CHECK( boost::fibers::channel_op_status::success == c.try_pop( v2) );
    BOOST_CHECK( c.is_empty() );
    BOOST_CHECK_EQUAL( v1, v2);
}

void test_try_pop_closed()
{
    boost::fibers::bounded_channel< int > c( 10);
    BOOST_CHECK( c.is_empty() );
    int v1 = 2, v2 = 0;
    BOOST_CHECK( boost::fibers::channel_op_status::success == c.push( v1) );
    BOOST_CHECK( ! c.is_empty() );
    BOOST_CHECK( ! c.is_closed() );
    c.close();
    BOOST_CHECK( c.is_closed() );
    BOOST_CHECK( boost::fibers::channel_op_status::success == c.try_pop( v2) );
    BOOST_CHECK( c.is_empty() );
    BOOST_CHECK_EQUAL( v1, v2);
    BOOST_CHECK( boost::fibers::channel_op_status::closed == c.try_pop( v2) );
}

void test_try_pop_success()
{
    boost::fibers::bounded_channel< int > c( 10);
    BOOST_CHECK( c.is_empty() );
    int v1 = 2, v2 = 0;
    boost::fibers::fiber f1([&c,&v2](){
        while ( boost::fibers::channel_op_status::success != c.try_pop( v2) );
        BOOST_CHECK( c.is_empty() );
    });
    boost::fibers::fiber f2([&c,v1](){
        BOOST_CHECK( boost::fibers::channel_op_status::success == c.push( v1) );
        BOOST_CHECK( ! c.is_empty() );
    });
    f1.join();
    BOOST_CHECK( c.is_empty() );
    f2.join();
    BOOST_CHECK( c.is_empty() );
    BOOST_CHECK_EQUAL( v1, v2);
}

void test_pop_wait_for()
{
    boost::fibers::bounded_channel< int > c( 10);
    BOOST_CHECK( c.is_empty() );
    int v1 = 2, v2 = 0;
    BOOST_CHECK( boost::fibers::channel_op_status::success == c.push( v1) );
    BOOST_CHECK( ! c.is_empty() );
    BOOST_CHECK( boost::fibers::channel_op_status::success == c.pop_wait_for( v2, std::chrono::seconds( 1) ) );
    BOOST_CHECK( c.is_empty() );
    BOOST_CHECK_EQUAL( v1, v2);
}

void test_pop_wait_for_closed()
{
    boost::fibers::bounded_channel< int > c( 10);
    BOOST_CHECK( c.is_empty() );
    int v1 = 2, v2 = 0;
    BOOST_CHECK( boost::fibers::channel_op_status::success == c.push( v1) );
    BOOST_CHECK( ! c.is_empty() );
    BOOST_CHECK( ! c.is_closed() );
    c.close();
    BOOST_CHECK( c.is_closed() );
    BOOST_CHECK( boost::fibers::channel_op_status::success == c.pop_wait_for( v2, std::chrono::seconds( 1) ) );
    BOOST_CHECK( c.is_empty() );
    BOOST_CHECK_EQUAL( v1, v2);
    BOOST_CHECK( boost::fibers::channel_op_status::closed == c.pop_wait_for( v2, std::chrono::seconds( 1) ) );
}

void test_pop_wait_for_success()
{
    boost::fibers::bounded_channel< int > c( 10);
    BOOST_CHECK( c.is_empty() );
    int v1 = 2, v2 = 0;
    boost::fibers::fiber f1([&c,&v2](){
        BOOST_CHECK( boost::fibers::channel_op_status::success == c.pop_wait_for( v2, std::chrono::seconds( 1) ) );
        BOOST_CHECK( c.is_empty() );
    });
    boost::fibers::fiber f2([&c,v1](){
        BOOST_CHECK( boost::fibers::channel_op_status::success == c.push( v1) );
        BOOST_CHECK( ! c.is_empty() );
    });
    f1.join();
    BOOST_CHECK( c.is_empty() );
    f2.join();
    BOOST_CHECK( c.is_empty() );
    BOOST_CHECK_EQUAL( v1, v2);
}

void test_pop_wait_for_timeout()
{
    boost::fibers::bounded_channel< int > c( 10);
    BOOST_CHECK( c.is_empty() );
    int v = 0;
    boost::fibers::fiber f([&c,&v](){
        BOOST_CHECK( boost::fibers::channel_op_status::timeout == c.pop_wait_for( v, std::chrono::seconds( 1) ) );
        BOOST_CHECK( c.is_empty() );
    });
    f.join();
}

void test_pop_wait_until()
{
    boost::fibers::bounded_channel< int > c( 10);
    BOOST_CHECK( c.is_empty() );
    int v1 = 2, v2 = 0;
    BOOST_CHECK( boost::fibers::channel_op_status::success == c.push( v1) );
    BOOST_CHECK( ! c.is_empty() );
    BOOST_CHECK( boost::fibers::channel_op_status::success == c.pop_wait_until( v2,
            std::chrono::system_clock::now() + std::chrono::seconds( 1) ) );
    BOOST_CHECK( c.is_empty() );
    BOOST_CHECK_EQUAL( v1, v2);
}

void test_pop_wait_until_closed()
{
    boost::fibers::bounded_channel< int > c( 10);
    BOOST_CHECK( c.is_empty() );
    int v1 = 2, v2 = 0;
    BOOST_CHECK( boost::fibers::channel_op_status::success == c.push( v1) );
    BOOST_CHECK( ! c.is_empty() );
    BOOST_CHECK( ! c.is_closed() );
    c.close();
    BOOST_CHECK( c.is_closed() );
    BOOST_CHECK( boost::fibers::channel_op_status::success == c.pop_wait_until( v2,
            std::chrono::system_clock::now() + std::chrono::seconds( 1) ) );
    BOOST_CHECK( c.is_empty() );
    BOOST_CHECK_EQUAL( v1, v2);
    BOOST_CHECK( boost::fibers::channel_op_status::closed == c.pop_wait_until( v2,
            std::chrono::system_clock::now() + std::chrono::seconds( 1) ) );
}

void test_pop_wait_until_success()
{
    boost::fibers::bounded_channel< int > c( 10);
    BOOST_CHECK( c.is_empty() );
    int v1 = 2, v2 = 0;
    boost::fibers::fiber f1([&c,&v2](){
        BOOST_CHECK( boost::fibers::channel_op_status::success == c.pop_wait_until( v2,
                    std::chrono::system_clock::now() + std::chrono::seconds( 1) ) );
        BOOST_CHECK( c.is_empty() );
    });
    boost::fibers::fiber f2([&c,v1](){
        BOOST_CHECK( boost::fibers::channel_op_status::success == c.push( v1) );
        BOOST_CHECK( ! c.is_empty() );
    });
    f1.join();
    BOOST_CHECK( c.is_empty() );
    f2.join();
    BOOST_CHECK( c.is_empty() );
    BOOST_CHECK_EQUAL( v1, v2);
}

void test_pop_wait_until_timeout()
{
    boost::fibers::bounded_channel< int > c( 10);
    BOOST_CHECK( c.is_empty() );
    int v = 0;
    boost::fibers::fiber f([&c,&v](){
        BOOST_CHECK( boost::fibers::channel_op_status::timeout == c.pop_wait_until( v,
                    std::chrono::system_clock::now() + std::chrono::seconds( 1) ) );
        BOOST_CHECK( c.is_empty() );
    });
    f.join();
}

void test_moveable()
{
    boost::fibers::bounded_channel< moveable > c( 10);
    BOOST_CHECK( c.is_empty() );
    moveable m1( 3), m2;
    BOOST_CHECK( m1.state);
    BOOST_CHECK( ! m2.state);
    BOOST_CHECK( boost::fibers::channel_op_status::success == c.push( std::move( m1) ) );
    BOOST_CHECK( ! m1.state);
    BOOST_CHECK( ! m2.state);
    BOOST_CHECK( ! c.is_empty() );
    BOOST_CHECK( boost::fibers::channel_op_status::success == c.pop( m2) );
    BOOST_CHECK( ! m1.state);
    BOOST_CHECK( m2.state);
    BOOST_CHECK_EQUAL( 3, m2.value);
    BOOST_CHECK( c.is_empty() );
}

boost::unit_test::test_suite * init_unit_test_suite( int, char* [])
{
    boost::unit_test::test_suite * test =
        BOOST_TEST_SUITE("Boost.Fiber: bounded_channel test suite");

     test->add( BOOST_TEST_CASE( & test_push) );
     test->add( BOOST_TEST_CASE( & test_push_closed) );
     test->add( BOOST_TEST_CASE( & test_pop) );
     test->add( BOOST_TEST_CASE( & test_pop_closed) );
     test->add( BOOST_TEST_CASE( & test_pop_success) );
     test->add( BOOST_TEST_CASE( & test_value_pop) );
     test->add( BOOST_TEST_CASE( & test_value_pop_closed) );
     test->add( BOOST_TEST_CASE( & test_value_pop_success) );
     test->add( BOOST_TEST_CASE( & test_try_pop) );
     test->add( BOOST_TEST_CASE( & test_try_pop_closed) );
     test->add( BOOST_TEST_CASE( & test_try_pop_success) );
     test->add( BOOST_TEST_CASE( & test_pop_wait_for) );
     test->add( BOOST_TEST_CASE( & test_pop_wait_for_closed) );
     test->add( BOOST_TEST_CASE( & test_pop_wait_for_success) );
     test->add( BOOST_TEST_CASE( & test_pop_wait_for_timeout) );
     test->add( BOOST_TEST_CASE( & test_pop_wait_until) );
     test->add( BOOST_TEST_CASE( & test_pop_wait_until_closed) );
     test->add( BOOST_TEST_CASE( & test_pop_wait_until_success) );
     test->add( BOOST_TEST_CASE( & test_pop_wait_until_timeout) );
     test->add( BOOST_TEST_CASE( & test_moveable) );

    return test;
}
