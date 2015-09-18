
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <sstream>
#include <string>

#include <boost/assert.hpp>
#include <boost/test/unit_test.hpp>

#include <boost/fiber/all.hpp>

int value1 = 0;
std::string value2 = "";

void fn1() {
    value1 = 1;
}

void fn2( int i, std::string const& s) {
    value1 = i;
    value2 = s;
}

void fn3( int & i) {
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

struct X {
    int value;

    void foo( int i) {
        value = i;
    }
};

class copyable {
public:
    bool    state;
    int     value;

    copyable() :
        state( false),
        value( -1) {
    }

    copyable( int v) :
        state( true),
        value( v) {
    }

    void operator()() {
        value1 = value;
    }
};

class moveable {
public:
    bool    state;
    int     value;

    moveable() :
        state( false),
        value( -1) {
    }

    moveable( int v) :
        state( true),
        value( v) {
    }

    moveable( moveable && other) :
        state( other.state),
        value( other.value) {
        other.state = false;
        other.value = -1;
    }

    moveable & operator=( moveable && other) {
        if ( this == & other) return * this;
        state = other.state;
        value = other.value;
        other.state = false;
        other.value = -1;
        return * this;
    }

    moveable( moveable const& other) = delete;
    moveable & operator=( moveable const& other) = delete;

    void operator()() {
        value1 = value;
    }
};

void test_scheduler_dtor() {
    boost::fibers::context * ctx(
        boost::fibers::context::active() );
    (void)ctx;
}

void test_join_fn() {
    {
        value1 = 0;
        boost::fibers::fiber f( fn1);
        f.join();
        BOOST_CHECK_EQUAL( value1, 1);
    }
    {
        value1 = 0;
        value2 = "";
        boost::fibers::fiber f( fn2, 3, "abc");
        f.join();
        BOOST_CHECK_EQUAL( value1, 3);
        BOOST_CHECK_EQUAL( value2, "abc");
    }
}

void test_join_memfn() {
    X x = {0};
    BOOST_CHECK_EQUAL( x.value, 0);
    boost::fibers::fiber( & X::foo, std::ref( x), 3).join();
    BOOST_CHECK_EQUAL( x.value, 3);
}

void test_join_copyable() {
    value1 = 0;
    copyable cp( 3);
    BOOST_CHECK( cp.state);
    BOOST_CHECK_EQUAL( value1, 0);
    boost::fibers::fiber f( cp);
    f.join();
    BOOST_CHECK( cp.state);
    BOOST_CHECK_EQUAL( value1, 3);
}

void test_join_moveable() {
    value1 = 0;
    moveable mv( 7);
    BOOST_CHECK( mv.state);
    BOOST_CHECK_EQUAL( value1, 0);
    boost::fibers::fiber f( std::move( mv) );
    f.join();
    BOOST_CHECK( ! mv.state);
    BOOST_CHECK_EQUAL( value1, 7);
}

void test_move_fiber() {
    boost::fibers::fiber f1;
    BOOST_CHECK( ! f1);
    boost::fibers::fiber f2( fn1);
    BOOST_CHECK( f2);
    f1 = std::move( f2);
    BOOST_CHECK( f1);
    BOOST_CHECK( ! f2);
    f1.join();
}

void test_id() {
    boost::fibers::fiber f1;
    boost::fibers::fiber f2( fn1);
    BOOST_CHECK( ! f1);
    BOOST_CHECK( f2);

    BOOST_CHECK_EQUAL( boost::fibers::fiber::id(), f1.get_id() );
    BOOST_CHECK( boost::fibers::fiber::id() != f2.get_id() );

    boost::fibers::fiber f3( fn1);
    BOOST_CHECK( f2.get_id() != f3.get_id() );

    f1 = std::move( f2);
    BOOST_CHECK( f1);
    BOOST_CHECK( ! f2);

    BOOST_CHECK( boost::fibers::fiber::id() != f1.get_id() );
    BOOST_CHECK_EQUAL( boost::fibers::fiber::id(), f2.get_id() );

    BOOST_CHECK( ! f2.joinable() );

    f1.join();
    f3.join();
}

void test_yield() {
    int v1 = 0, v2 = 0;
    BOOST_CHECK_EQUAL( 0, v1);
    BOOST_CHECK_EQUAL( 0, v2);
    boost::fibers::fiber f1( fn3, std::ref( v1) );
    boost::fibers::fiber f2( fn3, std::ref( v2) );
    f1.join();
    f2.join();
    BOOST_CHECK( ! f1);
    BOOST_CHECK( ! f2);
    BOOST_CHECK_EQUAL( 8, v1);
    BOOST_CHECK_EQUAL( 8, v2);
}

boost::unit_test::test_suite * init_unit_test_suite( int, char* []) {
    boost::unit_test::test_suite * test =
        BOOST_TEST_SUITE("Boost.Fiber: fiber test suite");

     test->add( BOOST_TEST_CASE( & test_scheduler_dtor) );
     test->add( BOOST_TEST_CASE( & test_join_fn) );
     test->add( BOOST_TEST_CASE( & test_join_memfn) );
     test->add( BOOST_TEST_CASE( & test_join_copyable) );
     test->add( BOOST_TEST_CASE( & test_join_moveable) );
     test->add( BOOST_TEST_CASE( & test_move_fiber) );
     test->add( BOOST_TEST_CASE( & test_move_fiber) );
     test->add( BOOST_TEST_CASE( & test_yield) );

    return test;
}
