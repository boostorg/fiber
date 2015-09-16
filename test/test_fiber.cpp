
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

void fn1() {
    value1 = 1;
}

void test_scheduler_dtor() {
    boost::fibers::context * ctx(
        boost::fibers::context::active() );
    (void)ctx;
}

void test_join() {
    value1 = 0;
    boost::fibers::fiber f( fn1);
    f.join();
    BOOST_CHECK_EQUAL( value1, 1);
}

boost::unit_test::test_suite * init_unit_test_suite( int, char* []) {
    boost::unit_test::test_suite * test =
        BOOST_TEST_SUITE("Boost.Fiber: fiber test suite");

     test->add( BOOST_TEST_CASE( & test_scheduler_dtor) );
     test->add( BOOST_TEST_CASE( & test_join) );

    return test;
}
