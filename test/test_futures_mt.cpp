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

typedef boost::shared_ptr< boost::fibers::packaged_task< int() > >  packaged_task_t;

int fn( int i)
{ return i; }

void exec( packaged_task_t pt)
{
    boost::fibers::fiber( boost::move( * pt) ).join();
}

boost::fibers::future< int > async( int i)
{
    packaged_task_t pt(
        new boost::fibers::packaged_task< int() >(
            boost::bind( fn, i) ) );
    boost::fibers::future< int > f( pt->get_future() );
    boost::thread( boost::bind( exec, pt) ).detach();
    return boost::move( f);
}

void test_async()
{
    int i = 3;
    boost::fibers::future< int > f = async( i);
    int result = f.get();
    BOOST_CHECK_EQUAL( i, result);
}

boost::unit_test_framework::test_suite* init_unit_test_suite(int, char*[])
{
    boost::unit_test_framework::test_suite* test =
        BOOST_TEST_SUITE("Boost.Fiber: futures-mt test suite");

    for ( int i = 0; i < 50; ++i)
    { test->add(BOOST_TEST_CASE(test_async)); }

    return test;
}
