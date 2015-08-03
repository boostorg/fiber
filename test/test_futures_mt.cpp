//  (C) Copyright 2008-10 Anthony Williams 
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <utility>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>

#include <boost/fiber/all.hpp>
#include <boost/test/unit_test.hpp>

int fn( int i)
{ return i; }

boost::fibers::future< int > async( int i)
{
    boost::fibers::packaged_task< int() > pt( std::bind( fn, i) );
    boost::fibers::future< int > f( pt.get_future() );
    std::thread( [pt=std::move( pt)] () mutable -> decltype( auto) { boost::fibers::fiber( std::move( pt) ).join(); } ).detach();
    return std::move( f);
}

void test_async()
{
    int i = 3;
    boost::fibers::future< int > f = async( i);
    int result = f.get();
    BOOST_CHECK_EQUAL( i, result);
}

void test_dummy() {}

boost::unit_test_framework::test_suite* init_unit_test_suite(int, char*[])
{
    boost::unit_test_framework::test_suite* test =
        BOOST_TEST_SUITE("Boost.Fiber: futures-mt test suite");

#if ! defined(BOOST_FIBERS_NO_ATOMICS)
    for ( int i = 0; i < 50; ++i)
    { test->add(BOOST_TEST_CASE(test_async)); }
#else
    test->add(BOOST_TEST_CASE(test_dummy));
#endif

    return test;
}
