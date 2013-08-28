// Copyright (C) 2011 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// <boost/thread/future.hpp>

// class future<R>

// template<typename F>
// auto then(F&& func) -> BOOST_THREAD_FUTURE<decltype(func(*this))>;

#include <boost/test/unit_test.hpp>

#include <boost/fiber/all.hpp>

int p1()
{
  return 1;
}

int p2(boost::fibers::unique_future<int>& f)
{
  return 2 * f.get();
}

void test_then()
{
    boost::fibers::round_robin ds;
    boost::fibers::set_scheduling_algorithm( & ds);

    {
        boost::fibers::unique_future<int> f1 = boost::fibers::async(p1);
        boost::fibers::unique_future<int> f2 = f1.then(p2);
        BOOST_CHECK_EQUAL( 2, f2.get());
    }
    {
        boost::fibers::unique_future<int> f2 = boost::fibers::async(p1).then(p2);
        BOOST_CHECK_EQUAL( 2, f2.get());
    }
    {
        boost::fibers::unique_future<int> f1 = boost::fibers::async(p1);
        boost::fibers::unique_future<int> f2 = f1.then(p2).then(p2);
        BOOST_CHECK_EQUAL( 4, f2.get());
    }
    {
        boost::fibers::unique_future<int> f2 = boost::fibers::async(p1).then(p2).then(p2);
        BOOST_CHECK_EQUAL( 4, f2.get());
    }
}

boost::unit_test_framework::test_suite* init_unit_test_suite(int, char*[])
{
    boost::unit_test_framework::test_suite* test =
        BOOST_TEST_SUITE("Boost.Fiber: unique_future::then() test suite");

    test->add(BOOST_TEST_CASE(test_then));

    return test;
}
