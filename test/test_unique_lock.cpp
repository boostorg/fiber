
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// This test is based on the tests of Boost.Thread 

#include <cstdlib>
#include <iostream>
#include <map>
#include <mutex>
#include <stdexcept>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <boost/fiber/all.hpp>

struct dummy_mutex
{
    bool is_locked;

    dummy_mutex() :
        is_locked( false)
    {}

    void lock()
    { is_locked = true; }

    bool try_lock()
    {
        if ( is_locked)
            return false;
        is_locked = true;
        return true;
    }

    void unlock()
    { is_locked = false; }
};

void lock()
{
    boost::fibers::mutex mtx;
    std::unique_lock< boost::fibers::mutex > lk( mtx);

    BOOST_CHECK( lk);
    BOOST_CHECK( lk.owns_lock() );

    lk.unlock();

    BOOST_CHECK( ! lk);
    BOOST_CHECK( ! lk.owns_lock() );
}

void defer_lock()
{
    boost::fibers::mutex mtx;
    std::unique_lock< boost::fibers::mutex > lk( mtx, std::defer_lock);

    BOOST_CHECK( ! lk);
    BOOST_CHECK( ! lk.owns_lock() );

    lk.lock();

    BOOST_CHECK( lk);
    BOOST_CHECK( lk.owns_lock() );
}

void adopt_lock()
{
    boost::fibers::mutex mtx;
    mtx.lock();
    std::unique_lock< boost::fibers::mutex > lk( mtx, std::adopt_lock);

    BOOST_CHECK( lk);
    BOOST_CHECK( lk.owns_lock() );
}

void try_lock()
{
    boost::fibers::mutex mtx;
    std::unique_lock< boost::fibers::mutex > lk( mtx, std::defer_lock);

    BOOST_CHECK( ! lk);
    BOOST_CHECK( ! lk.owns_lock() );

    lk.try_lock();

    BOOST_CHECK( lk);
    BOOST_CHECK( lk.owns_lock() );
}

void default_ctor()
{
    std::unique_lock< boost::fibers::mutex > lk;

    BOOST_CHECK( ! lk);
    BOOST_CHECK( ! lk.owns_lock() );
}

void lock_concept()
{
    boost::fibers::mutex mtx1, mtx2, mtx3;

    std::unique_lock< boost::fibers::mutex > lk1( mtx1, std::defer_lock),
        lk2( mtx2, std::defer_lock),
        lk3( mtx3, std::defer_lock);

    BOOST_CHECK( ! lk1.owns_lock() );
    BOOST_CHECK( ! lk2.owns_lock() );
    BOOST_CHECK( ! lk3.owns_lock() );
    
    std::lock( lk1, lk2, lk3);
    
    BOOST_CHECK( lk1.owns_lock() );
    BOOST_CHECK( lk2.owns_lock() );
    BOOST_CHECK( lk3.owns_lock() );
}

void try_lock_concept()
{
    dummy_mutex mtx1, mtx2;
    mtx2.lock();

    std::unique_lock< dummy_mutex > lk1( mtx1, std::defer_lock),
        lk2( mtx2, std::defer_lock);

    int res = std::try_lock( lk1, lk2);
    
    BOOST_CHECK( res == 1);
    BOOST_CHECK( ! mtx1.is_locked);
    BOOST_CHECK( mtx2.is_locked);
    BOOST_CHECK( ! lk1.owns_lock() );
    BOOST_CHECK( ! lk2.owns_lock() );
}

void swap()
{
    boost::fibers::mutex mtx1, mtx2;

    std::unique_lock< boost::fibers::mutex > lk1( mtx1), lk2( mtx2);

    BOOST_CHECK_EQUAL( lk1.mutex(), & mtx1);
    BOOST_CHECK_EQUAL( lk2.mutex(), & mtx2);

    lk1.swap( lk2);

    BOOST_CHECK_EQUAL( lk1.mutex(), & mtx2);
    BOOST_CHECK_EQUAL( lk2.mutex(), & mtx1);
}

void test_lock()
{
    lock();
}

void test_defer_lock()
{
    defer_lock();
}

void test_adopt_lock()
{
    adopt_lock();
}

void test_try_lock()
{
    try_lock();
}

void test_default_ctor()
{
    default_ctor();
}

void test_lock_concept()
{
    lock_concept();
}

void test_try_lock_concept()
{
    try_lock_concept();
}

void test_swap()
{
    swap();
}

boost::unit_test::test_suite * init_unit_test_suite( int, char* [])
{
    boost::unit_test::test_suite * test =
        BOOST_TEST_SUITE("Boost.Fiber: unique lock test suite");

    test->add( BOOST_TEST_CASE( & test_lock) );
    test->add( BOOST_TEST_CASE( & test_defer_lock) );
    test->add( BOOST_TEST_CASE( & test_adopt_lock) );
    test->add( BOOST_TEST_CASE( & test_try_lock) );
    test->add( BOOST_TEST_CASE( & test_default_ctor) );
    test->add( BOOST_TEST_CASE( & test_lock_concept) );
    test->add( BOOST_TEST_CASE( & test_try_lock_concept) );
    test->add( BOOST_TEST_CASE( & test_swap) );

    return test;
}
