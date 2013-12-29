//  (C) Copyright 2008-10 Anthony Williams 
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <utility>
#include <memory>
#include <stdexcept>
#include <string>

#include <boost/bind.hpp>
#include <boost/fiber/all.hpp>
#include <boost/move/move.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/thread.hpp>

boost::fibers::packaged_task< int() > pt;

int fn( int i)
{ return i; }

void exec()
{
    boost::fibers::round_robin rr;
    boost::fibers::set_scheduling_algorithm( & rr);
    boost::fibers::fiber( boost::move( pt) ).join();
}

boost::fibers::future< int > async( int i)
{
    boost::fibers::packaged_task< int() > tmp( boost::bind( fn, i) );
    boost::fibers::future< int > f( tmp.get_future() );
    pt = boost::move( tmp);
    boost::thread( boost::bind( exec) ).detach();
    return boost::move( f);
}

void test_async()
{
    int i = 3;
    boost::fibers::future< int > f = async( i);
    BOOST_CHECK( f);
    BOOST_CHECK( f.valid() );

    BOOST_CHECK_EQUAL( i, f.get() );

    BOOST_CHECK( ! f);
    BOOST_CHECK( ! f.valid() );
}

boost::unit_test_framework::test_suite* init_unit_test_suite(int, char*[])
{
    boost::unit_test_framework::test_suite* test =
        BOOST_TEST_SUITE("Boost.Fiber: futures-mt test suite");

    for ( int i = 0; i < 50; ++i)
    { test->add(BOOST_TEST_CASE(test_async)); }

    return test;
}
