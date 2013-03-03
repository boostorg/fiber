//  (C) Copyright 2008-10 Anthony Williams 
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <utility>
#include <memory>
#include <string>

#include <boost/bind.hpp>
#include <boost/move/move.hpp>
#include <boost/fiber/all.hpp>
#include <boost/test/unit_test.hpp>

struct my_exception
{};

void fn1( boost::fibers::promise< int > * p, int i)
{
    boost::this_fiber::yield();
    p->set_value( i);
}

void fn2()
{
    boost::fibers::promise< int > p;
    boost::fibers::future< int > f( p.get_future() );
    boost::this_fiber::yield();
    boost::fibers::fiber( boost::bind( fn1, & p, 7) ).detach();
    boost::this_fiber::yield();
    BOOST_CHECK( 7 == f.get() );
}

// promise
void test_promise_create()
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    // use std::allocator<> as default
    boost::fibers::promise< int > p1;
    BOOST_CHECK( p1);

    // use std::allocator<> as user defined
    std::allocator< boost::fibers::promise< int > > alloc;
    boost::fibers::promise< int > p2( alloc);
    BOOST_CHECK( p2);
}

void test_promise_create_void()
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    // use std::allocator<> as default
    boost::fibers::promise< void > p1;
    BOOST_CHECK( p1);

    // use std::allocator<> as user defined
    std::allocator< boost::fibers::promise< void > > alloc;
    boost::fibers::promise< void > p2( alloc);
    BOOST_CHECK( p2);
}

void test_promise_move()
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    boost::fibers::promise< int > p1;
    BOOST_CHECK( p1);

    // move construction
    boost::fibers::promise< int > p2( boost::move( p1) );
    BOOST_CHECK( ! p1);
    BOOST_CHECK( p2);

    // move assigment
    p1 = boost::move( p2);
    BOOST_CHECK( p1);
    BOOST_CHECK( ! p2);
}

void test_promise_move_void()
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    boost::fibers::promise< void > p1;
    BOOST_CHECK( p1);

    // move construction
    boost::fibers::promise< void > p2( boost::move( p1) );
    BOOST_CHECK( ! p1);
    BOOST_CHECK( p2);

    // move assigment
    p1 = boost::move( p2);
    BOOST_CHECK( p1);
    BOOST_CHECK( ! p2);
}

void test_promise_swap()
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    boost::fibers::promise< int > p1;
    BOOST_CHECK( p1);

    // move construction
    boost::fibers::promise< int > p2( boost::move( p1) );
    BOOST_CHECK( ! p1);
    BOOST_CHECK( p2);

    // swap
    p1.swap( p2);
    BOOST_CHECK( p1);
    BOOST_CHECK( ! p2);
}

void test_promise_swap_void()
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    boost::fibers::promise< void > p1;
    BOOST_CHECK( p1);

    // move construction
    boost::fibers::promise< void > p2( boost::move( p1) );
    BOOST_CHECK( ! p1);
    BOOST_CHECK( p2);

    // swap
    p1.swap( p2);
    BOOST_CHECK( p1);
    BOOST_CHECK( ! p2);
}

void test_promise_get_future()
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    boost::fibers::promise< int > p1;
    BOOST_CHECK( p1);

    // retrieve future
    boost::fibers::future< int > f1 = p1.get_future();
    BOOST_CHECK( f1);
    BOOST_CHECK( f1.valid() );

    // retrieve future a second time
    bool thrown = false;
    try
    { f1 = p1.get_future(); }
    catch ( boost::fibers::future_already_retrieved const&)
    { thrown = true; }
    BOOST_CHECK( thrown);

    // move construction
    boost::fibers::promise< int > p2( boost::move( p1) );
    BOOST_CHECK( ! p1);
    BOOST_CHECK( p2);

    // retrieve future from uninitialized
    thrown = false;
    try
    { f1 = p1.get_future(); }
    catch ( boost::fibers::promise_uninitialized const&)
    { thrown = true; }
    BOOST_CHECK( thrown);
}

void test_promise_get_future_void()
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    boost::fibers::promise< void > p1;
    BOOST_CHECK( p1);

    // retrieve future
    boost::fibers::future< void > f1 = p1.get_future();
    BOOST_CHECK( f1);
    BOOST_CHECK( f1.valid() );

    // retrieve future a second time
    bool thrown = false;
    try
    { f1 = p1.get_future(); }
    catch ( boost::fibers::future_already_retrieved const&)
    { thrown = true; }
    BOOST_CHECK( thrown);

    // move construction
    boost::fibers::promise< void > p2( boost::move( p1) );
    BOOST_CHECK( ! p1);
    BOOST_CHECK( p2);

    // retrieve future from uninitialized
    thrown = false;
    try
    { f1 = p1.get_future(); }
    catch ( boost::fibers::promise_uninitialized const&)
    { thrown = true; }
    BOOST_CHECK( thrown);
}

void test_promise_set_value()
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    // promise takes a copyable as return type
    boost::fibers::promise< int > p1;
    BOOST_CHECK( p1);
    boost::fibers::future< int > f1 = p1.get_future();
    BOOST_CHECK( f1);
    BOOST_CHECK( f1.valid() );

    // copy value
    p1.set_value( 7);
    BOOST_CHECK( 7 == f1.get() );

    // set value a second time
    bool thrown = false;
    try
    { p1.set_value( 11); }
    catch ( boost::fibers::promise_already_satisfied const&)
    { thrown = true; }
    BOOST_CHECK( thrown);

    //TODO: promise takes a reference as return type

    //TODO: promise takes a moveable-only as return type
}

void test_promise_set_value_void()
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    // promise takes a copyable as return type
    boost::fibers::promise< void > p1;
    BOOST_CHECK( p1);
    boost::fibers::future< void > f1 = p1.get_future();
    BOOST_CHECK( f1);
    BOOST_CHECK( f1.valid() );

    // set void
    p1.set_value();
    f1.get();

    // set value a second time
    bool thrown = false;
    try
    { p1.set_value(); }
    catch ( boost::fibers::promise_already_satisfied const&)
    { thrown = true; }
    BOOST_CHECK( thrown);
}

void test_promise_set_exception()
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    boost::fibers::promise< int > p1;
    BOOST_CHECK( p1);
    boost::fibers::future< int > f1 = p1.get_future();
    BOOST_CHECK( f1);
    BOOST_CHECK( f1.valid() );
    p1.set_exception( boost::copy_exception( my_exception() ) );

    // set exception a second time
    bool thrown = false;
    try
    { p1.set_exception( boost::copy_exception( my_exception() ) ); }
    catch ( boost::fibers::promise_already_satisfied const&)
    { thrown = true; }
    BOOST_CHECK( thrown);

    // set value
    thrown = false;
    try
    { p1.set_value( 11); }
    catch ( boost::fibers::promise_already_satisfied const&)
    { thrown = true; }
    BOOST_CHECK( thrown);
}

void test_promise_set_exception_void()
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    boost::fibers::promise< void > p1;
    BOOST_CHECK( p1);
    boost::fibers::future< void > f1 = p1.get_future();
    BOOST_CHECK( f1);
    BOOST_CHECK( f1.valid() );
    p1.set_exception( boost::copy_exception( my_exception() ) );

    // set exception a second time
    bool thrown = false;
    try
    { p1.set_exception( boost::copy_exception( my_exception() ) ); }
    catch ( boost::fibers::promise_already_satisfied const&)
    { thrown = true; }
    BOOST_CHECK( thrown);

    // set value
    thrown = false;
    try
    { p1.set_value(); }
    catch ( boost::fibers::promise_already_satisfied const&)
    { thrown = true; }
    BOOST_CHECK( thrown);
}

// future
void test_future_create()
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    // default constructed future is not valid
    boost::fibers::future< int > f1;
    BOOST_CHECK( ! f1);
    BOOST_CHECK( ! f1.valid() );

    // future retrieved from promise is valid (if it is the first)
    boost::fibers::promise< int > p2;
    boost::fibers::future< int > f2 = p2.get_future();
    BOOST_CHECK( f2);
    BOOST_CHECK( f2.valid() );
}

void test_future_create_void()
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    // default constructed future is not valid
    boost::fibers::future< void > f1;
    BOOST_CHECK( ! f1);
    BOOST_CHECK( ! f1.valid() );

    // future retrieved from promise is valid (if it is the first)
    boost::fibers::promise< void > p2;
    boost::fibers::future< void > f2 = p2.get_future();
    BOOST_CHECK( f2);
    BOOST_CHECK( f2.valid() );
}

void test_future_move()
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    // future retrieved from promise is valid (if it is the first)
    boost::fibers::promise< int > p1;
    boost::fibers::future< int > f1 = p1.get_future();
    BOOST_CHECK( f1);
    BOOST_CHECK( f1.valid() );

    // move construction
    boost::fibers::future< int > f2( boost::move( f1) );
    BOOST_CHECK( ! f1);
    BOOST_CHECK( ! f1.valid() );
    BOOST_CHECK( f2);
    BOOST_CHECK( f2.valid() );

    // move assignment
    f1 = boost::move( f2);
    BOOST_CHECK( f1);
    BOOST_CHECK( f1.valid() );
    BOOST_CHECK( ! f2);
    BOOST_CHECK( ! f2.valid() );
}

void test_future_move_void()
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    // future retrieved from promise is valid (if it is the first)
    boost::fibers::promise< void > p1;
    boost::fibers::future< void > f1 = p1.get_future();
    BOOST_CHECK( f1);
    BOOST_CHECK( f1.valid() );

    // move construction
    boost::fibers::future< void > f2( boost::move( f1) );
    BOOST_CHECK( ! f1);
    BOOST_CHECK( ! f1.valid() );
    BOOST_CHECK( f2);
    BOOST_CHECK( f2.valid() );

    // move assignment
    f1 = boost::move( f2);
    BOOST_CHECK( f1);
    BOOST_CHECK( f1.valid() );
    BOOST_CHECK( ! f2);
    BOOST_CHECK( ! f2.valid() );
}

void test_future_swap()
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    // future retrieved from promise is valid (if it is the first)
    boost::fibers::promise< int > p1;
    boost::fibers::future< int > f1 = p1.get_future();
    BOOST_CHECK( f1);
    BOOST_CHECK( f1.valid() );

    boost::fibers::future< int > f2;
    BOOST_CHECK( ! f2);
    BOOST_CHECK( ! f2.valid() );

    // swap
    f1.swap( f2);
    BOOST_CHECK( ! f1);
    BOOST_CHECK( ! f1.valid() );
    BOOST_CHECK( f2);
    BOOST_CHECK( f2.valid() );
}

void test_future_swap_void()
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    // future retrieved from promise is valid (if it is the first)
    boost::fibers::promise< void > p1;
    boost::fibers::future< void > f1 = p1.get_future();
    BOOST_CHECK( f1);
    BOOST_CHECK( f1.valid() );

    boost::fibers::future< void > f2;
    BOOST_CHECK( ! f2);
    BOOST_CHECK( ! f2.valid() );

    // swap
    f1.swap( f2);
    BOOST_CHECK( ! f1);
    BOOST_CHECK( ! f1.valid() );
    BOOST_CHECK( f2);
    BOOST_CHECK( f2.valid() );
}

void test_future_get()
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    // future retrieved from promise is valid (if it is the first)
    boost::fibers::promise< int > p1;
    p1.set_value( 7);

    boost::fibers::future< int > f1 = p1.get_future();
    BOOST_CHECK( f1);
    BOOST_CHECK( f1.valid() );

    // get
    BOOST_CHECK( 7 == f1.get() );
    BOOST_CHECK( ! f1.valid() );

    //TODO: future gets a reference as return type

    //TODO: future gets a moveable-only as return type

    // throw broken_promise if promise is destroyed without set
    {
        boost::fibers::promise< int > p2;
        f1 = p2.get_future();
    }
    bool thrown = false;
    try
    { f1.get(); }
    catch ( boost::fibers::broken_promise const&)
    { thrown = true; }
    BOOST_CHECK( ! f1.valid() );
    BOOST_CHECK( thrown);
}

void test_future_get_void()
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    // future retrieved from promise is valid (if it is the first)
    boost::fibers::promise< void > p1;
    p1.set_value();

    boost::fibers::future< void > f1 = p1.get_future();
    BOOST_CHECK( f1);
    BOOST_CHECK( f1.valid() );

    // get
    f1.get();
    BOOST_CHECK( ! f1.valid() );

    // throw broken_promise if promise is destroyed without set
    {
        boost::fibers::promise< void > p2;
        f1 = p2.get_future();
    }
    bool thrown = false;
    try
    { f1.get(); }
    catch ( boost::fibers::broken_promise const&)
    { thrown = true; }
    BOOST_CHECK( ! f1.valid() );
    BOOST_CHECK( thrown);
}

void test_future_wait()
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    // future retrieved from promise is valid (if it is the first)
    boost::fibers::promise< int > p1;
    boost::fibers::future< int > f1 = p1.get_future();

    // wait on future
    p1.set_value( 7);
    f1.wait();
    BOOST_CHECK( 7 == f1.get() );
}

void test_future_wait_void()
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    // future retrieved from promise is valid (if it is the first)
    boost::fibers::promise< void > p1;
    boost::fibers::future< void > f1 = p1.get_future();

    // wait on future
    p1.set_value();
    f1.wait();
    f1.get();
    BOOST_CHECK( ! f1.valid() );
}

void test_future_wait_with_fiber_1()
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    boost::fibers::promise< int > p1;
    boost::fibers::fiber(
        boost::bind( fn1, & p1, 7) ).detach();

    boost::fibers::future< int > f1 = p1.get_future();

    // wait on future
    BOOST_CHECK( 7 == f1.get() );
}

void test_future_wait_with_fiber_2()
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    boost::fibers::fiber( fn2).join();
}

boost::unit_test_framework::test_suite* init_unit_test_suite(int, char*[])
{
    boost::unit_test_framework::test_suite* test =
        BOOST_TEST_SUITE("Boost.Fiber: futures test suite");

    test->add(BOOST_TEST_CASE(test_promise_create));
    test->add(BOOST_TEST_CASE(test_promise_create_void));
    test->add(BOOST_TEST_CASE(test_promise_move));
    test->add(BOOST_TEST_CASE(test_promise_move_void));
    test->add(BOOST_TEST_CASE(test_promise_swap));
    test->add(BOOST_TEST_CASE(test_promise_swap_void));
    test->add(BOOST_TEST_CASE(test_promise_get_future));
    test->add(BOOST_TEST_CASE(test_promise_get_future_void));
    test->add(BOOST_TEST_CASE(test_promise_set_value));
    test->add(BOOST_TEST_CASE(test_promise_set_value_void));
    test->add(BOOST_TEST_CASE(test_promise_set_exception));
    test->add(BOOST_TEST_CASE(test_promise_set_exception_void));

    test->add(BOOST_TEST_CASE(test_future_create));
    test->add(BOOST_TEST_CASE(test_future_create_void));
    test->add(BOOST_TEST_CASE(test_future_move));
    test->add(BOOST_TEST_CASE(test_future_move_void));
    test->add(BOOST_TEST_CASE(test_future_swap));
    test->add(BOOST_TEST_CASE(test_future_swap_void));
    test->add(BOOST_TEST_CASE(test_future_get));
    test->add(BOOST_TEST_CASE(test_future_get_void));
    test->add(BOOST_TEST_CASE(test_future_wait));
    test->add(BOOST_TEST_CASE(test_future_wait_void));
    test->add(BOOST_TEST_CASE(test_future_wait_with_fiber_1));
    test->add(BOOST_TEST_CASE(test_future_wait_with_fiber_2));

    return test;
}
