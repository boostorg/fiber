//  (C) Copyright 2008-10 Anthony Williams 
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <utility>
#include <memory>
#include <stdexcept>
#include <string>

#include <boost/test/unit_test.hpp>

#include <boost/fiber/all.hpp>

int gi = 7;

struct my_exception : public std::runtime_error {
    my_exception() :
        std::runtime_error("my_exception") {
    }
};

struct A {
    A() = default;

    A( A const&) = delete;
    A( A &&) = default;

    A & operator=( A const&) = delete;
    A & operator=( A &&) = default;

    int value;
};

void fn1( boost::fibers::promise< int > * p, int i) {
    boost::this_fiber::yield();
    p->set_value( i);
}

void fn2() {
    boost::fibers::promise< int > p;
    boost::fibers::future< int > f( p.get_future() );
    boost::this_fiber::yield();
    boost::fibers::fiber( fn1, & p, 7).detach();
    boost::this_fiber::yield();
    BOOST_CHECK( 7 == f.get() );
}

int fn3() {
    return 3;
}

void fn4() {
}

int fn5() {
    boost::throw_exception( my_exception() );
    return 3;
}

void fn6() {
    boost::throw_exception( my_exception() );
}

int & fn7() {
    return gi;
}

int fn8( int i) {
    return i;
}

A fn9() {
     A a;
     a.value = 3;
     return std::move( a);
}

A fn10() {
    boost::throw_exception( my_exception() );
    return A();
}

// promise
void test_promise_create() {
    // use std::allocator<> as default
    boost::fibers::promise< int > p1;

    // use std::allocator<> as user defined
    std::allocator< boost::fibers::promise< int > > alloc;
    boost::fibers::promise< int > p2( std::allocator_arg,  alloc);
}

void test_promise_create_ref() {
    // use std::allocator<> as default
    boost::fibers::promise< int& > p1;

    // use std::allocator<> as user defined
    std::allocator< boost::fibers::promise< int& > > alloc;
    boost::fibers::promise< int& > p2( std::allocator_arg, alloc);
}

void test_promise_create_void() {
    // use std::allocator<> as default
    boost::fibers::promise< void > p1;

    // use std::allocator<> as user defined
    std::allocator< boost::fibers::promise< void > > alloc;
    boost::fibers::promise< void > p2( std::allocator_arg, alloc);
}

void test_promise_move() {
    boost::fibers::promise< int > p1;

    // move construction
    boost::fibers::promise< int > p2( std::move( p1) );

    // move assigment
    p1 = std::move( p2);
}

void test_promise_move_ref() {
    boost::fibers::promise< int& > p1;

    // move construction
    boost::fibers::promise< int& > p2( std::move( p1) );

    // move assigment
    p1 = std::move( p2);
}

void test_promise_move_void() {
    boost::fibers::promise< void > p1;

    // move construction
    boost::fibers::promise< void > p2( std::move( p1) );

    // move assigment
    p1 = std::move( p2);
}

void test_promise_swap() {
    boost::fibers::promise< int > p1;

    // move construction
    boost::fibers::promise< int > p2( std::move( p1) );

    // swap
    p1.swap( p2);
}

void test_promise_swap_ref() {
    boost::fibers::promise< int& > p1;

    // move construction
    boost::fibers::promise< int& > p2( std::move( p1) );

    // swap
    p1.swap( p2);
}

void test_promise_swap_void() {
    boost::fibers::promise< void > p1;

    // move construction
    boost::fibers::promise< void > p2( std::move( p1) );

    // swap
    p1.swap( p2);
}

void test_promise_get_future() {
    boost::fibers::promise< int > p1;

    // retrieve future
    boost::fibers::future< int > f1 = p1.get_future();
    BOOST_CHECK( f1.valid() );

    // retrieve future a second time
    bool thrown = false;
    try {
        f1 = p1.get_future();
    } catch ( boost::fibers::future_already_retrieved const&) {
        thrown = true;
    }
    BOOST_CHECK( thrown);

    // move construction
    boost::fibers::promise< int > p2( std::move( p1) );

    // retrieve future from uninitialized
    thrown = false;
    try {
        f1 = p1.get_future();
    } catch ( boost::fibers::promise_uninitialized const&) {
        thrown = true;
    }
    BOOST_CHECK( thrown);
}

void test_promise_get_future_ref() {
    boost::fibers::promise< int& > p1;

    // retrieve future
    boost::fibers::future< int& > f1 = p1.get_future();
    BOOST_CHECK( f1.valid() );

    // retrieve future a second time
    bool thrown = false;
    try {
        f1 = p1.get_future();
    } catch ( boost::fibers::future_already_retrieved const&) {
        thrown = true;
    }
    BOOST_CHECK( thrown);

    // move construction
    boost::fibers::promise< int& > p2( std::move( p1) );

    // retrieve future from uninitialized
    thrown = false;
    try {
        f1 = p1.get_future();
    } catch ( boost::fibers::promise_uninitialized const&) {
        thrown = true;
    }
    BOOST_CHECK( thrown);
}

void test_promise_get_future_void() {
    boost::fibers::promise< void > p1;

    // retrieve future
    boost::fibers::future< void > f1 = p1.get_future();
    BOOST_CHECK( f1.valid() );

    // retrieve future a second time
    bool thrown = false;
    try {
        f1 = p1.get_future();
    } catch ( boost::fibers::future_already_retrieved const&) {
        thrown = true;
    }
    BOOST_CHECK( thrown);

    // move construction
    boost::fibers::promise< void > p2( std::move( p1) );

    // retrieve future from uninitialized
    thrown = false;
    try {
        f1 = p1.get_future();
    } catch ( boost::fibers::promise_uninitialized const&) {
        thrown = true;
    }
    BOOST_CHECK( thrown);
}

void test_promise_set_value() {
    // promise takes a copyable as return type
    boost::fibers::promise< int > p1;
    boost::fibers::future< int > f1 = p1.get_future();
    BOOST_CHECK( f1.valid() );

    // copy value
    p1.set_value( 7);
    BOOST_CHECK( 7 == f1.get() );

    // set value a second time
    bool thrown = false;
    try {
        p1.set_value( 11);
    } catch ( boost::fibers::promise_already_satisfied const&) {
        thrown = true;
    }
    BOOST_CHECK( thrown);
}

void test_promise_set_value_move() {
    // promise takes a copyable as return type
    boost::fibers::promise< A > p1;
    boost::fibers::future< A > f1 = p1.get_future();
    BOOST_CHECK( f1.valid() );

    // move value
    A a1; a1.value = 7;
    p1.set_value( std::move( a1) );
    A a2 = f1.get();
    BOOST_CHECK( 7 == a2.value);

    // set value a second time
    bool thrown = false;
    try {
        A a;
        p1.set_value( std::move( a) );
    } catch ( boost::fibers::promise_already_satisfied const&) {
        thrown = true;
    }
    BOOST_CHECK( thrown);
}

void test_promise_set_value_ref() {
    // promise takes a reference as return type
    boost::fibers::promise< int& > p1;
    boost::fibers::future< int& > f1 = p1.get_future();
    BOOST_CHECK( f1.valid() );

    // copy value
    int i = 7;
    p1.set_value( i);
    int & j = f1.get();
    BOOST_CHECK( &i == &j);

    // set value a second time
    bool thrown = false;
    try {
        p1.set_value( i);
    } catch ( boost::fibers::promise_already_satisfied const&) {
        thrown = true;
    }
    BOOST_CHECK( thrown);
}

void test_promise_set_value_void() {
    // promise takes a copyable as return type
    boost::fibers::promise< void > p1;
    boost::fibers::future< void > f1 = p1.get_future();
    BOOST_CHECK( f1.valid() );

    // set void
    p1.set_value();
    f1.get();

    // set value a second time
    bool thrown = false;
    try {
        p1.set_value();
    } catch ( boost::fibers::promise_already_satisfied const&) {
        thrown = true;
    }
    BOOST_CHECK( thrown);
}

void test_promise_set_exception() {
    boost::fibers::promise< int > p1;
    boost::fibers::future< int > f1 = p1.get_future();
    BOOST_CHECK( f1.valid() );
    p1.set_exception( std::make_exception_ptr( my_exception() ) );

    // set exception a second time
    bool thrown = false;
    try {
        p1.set_exception( std::make_exception_ptr( my_exception() ) );
    } catch ( boost::fibers::promise_already_satisfied const&) {
        thrown = true;
    }
    BOOST_CHECK( thrown);

    // set value
    thrown = false;
    try
    { p1.set_value( 11); }
    catch ( boost::fibers::promise_already_satisfied const&)
    { thrown = true; }
    BOOST_CHECK( thrown);
}

void test_promise_set_exception_ref() {
    boost::fibers::promise< int& > p1;
    boost::fibers::future< int& > f1 = p1.get_future();
    BOOST_CHECK( f1.valid() );
    p1.set_exception( std::make_exception_ptr( my_exception() ) );

    // set exception a second time
    bool thrown = false;
    try {
        p1.set_exception( std::make_exception_ptr( my_exception() ) );
    } catch ( boost::fibers::promise_already_satisfied const&) {
        thrown = true;
    }
    BOOST_CHECK( thrown);

    // set value
    thrown = false;
    int i = 11;
    try {
        p1.set_value( i);
    } catch ( boost::fibers::promise_already_satisfied const&) {
        thrown = true;
    }
    BOOST_CHECK( thrown);
}

void test_promise_set_exception_void() {
    boost::fibers::promise< void > p1;
    boost::fibers::future< void > f1 = p1.get_future();
    BOOST_CHECK( f1.valid() );
    p1.set_exception( std::make_exception_ptr( my_exception() ) );

    // set exception a second time
    bool thrown = false;
    try {
        p1.set_exception( std::make_exception_ptr( my_exception() ) );
    } catch ( boost::fibers::promise_already_satisfied const&) {
        thrown = true;
    }
    BOOST_CHECK( thrown);

    // set value
    thrown = false;
    try {
        p1.set_value();
    } catch ( boost::fibers::promise_already_satisfied const&) {
        thrown = true;
    }
    BOOST_CHECK( thrown);
}

// future
void test_future_create() {
    // default constructed future is not valid
    boost::fibers::future< int > f1;
    BOOST_CHECK( ! f1.valid() );

    // future retrieved from promise is valid (if it is the first)
    boost::fibers::promise< int > p2;
    boost::fibers::future< int > f2 = p2.get_future();
    BOOST_CHECK( f2.valid() );
}

void test_future_create_ref() {
    // default constructed future is not valid
    boost::fibers::future< int& > f1;
    BOOST_CHECK( ! f1.valid() );

    // future retrieved from promise is valid (if it is the first)
    boost::fibers::promise< int& > p2;
    boost::fibers::future< int& > f2 = p2.get_future();
    BOOST_CHECK( f2.valid() );
}

void test_future_create_void() {
    // default constructed future is not valid
    boost::fibers::future< void > f1;
    BOOST_CHECK( ! f1.valid() );

    // future retrieved from promise is valid (if it is the first)
    boost::fibers::promise< void > p2;
    boost::fibers::future< void > f2 = p2.get_future();
    BOOST_CHECK( f2.valid() );
}

void test_future_move() {
    // future retrieved from promise is valid (if it is the first)
    boost::fibers::promise< int > p1;
    boost::fibers::future< int > f1 = p1.get_future();
    BOOST_CHECK( f1.valid() );

    // move construction
    boost::fibers::future< int > f2( std::move( f1) );
    BOOST_CHECK( ! f1.valid() );
    BOOST_CHECK( f2.valid() );

    // move assignment
    f1 = std::move( f2);
    BOOST_CHECK( f1.valid() );
    BOOST_CHECK( ! f2.valid() );
}

void test_future_move_ref() {
    // future retrieved from promise is valid (if it is the first)
    boost::fibers::promise< int& > p1;
    boost::fibers::future< int& > f1 = p1.get_future();
    BOOST_CHECK( f1.valid() );

    // move construction
    boost::fibers::future< int& > f2( std::move( f1) );
    BOOST_CHECK( ! f1.valid() );
    BOOST_CHECK( f2.valid() );

    // move assignment
    f1 = std::move( f2);
    BOOST_CHECK( f1.valid() );
    BOOST_CHECK( ! f2.valid() );
}

void test_future_move_void() {
    // future retrieved from promise is valid (if it is the first)
    boost::fibers::promise< void > p1;
    boost::fibers::future< void > f1 = p1.get_future();
    BOOST_CHECK( f1.valid() );

    // move construction
    boost::fibers::future< void > f2( std::move( f1) );
    BOOST_CHECK( ! f1.valid() );
    BOOST_CHECK( f2.valid() );

    // move assignment
    f1 = std::move( f2);
    BOOST_CHECK( f1.valid() );
    BOOST_CHECK( ! f2.valid() );
}

void test_future_get() {
    // future retrieved from promise is valid (if it is the first)
    boost::fibers::promise< int > p1;
    p1.set_value( 7);

    boost::fibers::future< int > f1 = p1.get_future();
    BOOST_CHECK( f1.valid() );

    // get
    BOOST_CHECK( ! f1.get_exception_ptr() );
    BOOST_CHECK( 7 == f1.get() );
    BOOST_CHECK( ! f1.valid() );

    // throw broken_promise if promise is destroyed without set
    {
        boost::fibers::promise< int > p2;
        f1 = p2.get_future();
    }
    bool thrown = false;
    try {
        f1.get();
    } catch ( boost::fibers::broken_promise const&) {
        thrown = true;
    }
    BOOST_CHECK( ! f1.valid() );
    BOOST_CHECK( thrown);
}

void test_future_get_move() {
    // future retrieved from promise is valid (if it is the first)
    boost::fibers::promise< A > p1;
    A a; a.value = 7;
    p1.set_value( std::move( a) );

    boost::fibers::future< A > f1 = p1.get_future();
    BOOST_CHECK( f1.valid() );

    // get
    BOOST_CHECK( ! f1.get_exception_ptr() );
    BOOST_CHECK( 7 == f1.get().value);
    BOOST_CHECK( ! f1.valid() );

    // throw broken_promise if promise is destroyed without set
    {
        boost::fibers::promise< A > p2;
        f1 = p2.get_future();
    }
    bool thrown = false;
    try {
        f1.get();
    } catch ( boost::fibers::broken_promise const&) {
        thrown = true;
    }
    BOOST_CHECK( ! f1.valid() );
    BOOST_CHECK( thrown);
}

void test_future_get_ref() {
    // future retrieved from promise is valid (if it is the first)
    boost::fibers::promise< int& > p1;
    int i = 7;
    p1.set_value( i);

    boost::fibers::future< int& > f1 = p1.get_future();
    BOOST_CHECK( f1.valid() );

    // get
    BOOST_CHECK( ! f1.get_exception_ptr() );
    int & j = f1.get();
    BOOST_CHECK( &i == &j);
    BOOST_CHECK( ! f1.valid() );

    // throw broken_promise if promise is destroyed without set
    {
        boost::fibers::promise< int& > p2;
        f1 = p2.get_future();
    }
    bool thrown = false;
    try {
        f1.get();
    } catch ( boost::fibers::broken_promise const&) {
        thrown = true;
    }
    BOOST_CHECK( ! f1.valid() );
    BOOST_CHECK( thrown);
}


void test_future_get_void() {
    // future retrieved from promise is valid (if it is the first)
    boost::fibers::promise< void > p1;
    p1.set_value();

    boost::fibers::future< void > f1 = p1.get_future();
    BOOST_CHECK( f1.valid() );

    // get
    BOOST_CHECK( ! f1.get_exception_ptr() );
    f1.get();
    BOOST_CHECK( ! f1.valid() );

    // throw broken_promise if promise is destroyed without set
    {
        boost::fibers::promise< void > p2;
        f1 = p2.get_future();
    }
    bool thrown = false;
    try {
        f1.get();
    } catch ( boost::fibers::broken_promise const&) {
        thrown = true;
    }
    BOOST_CHECK( ! f1.valid() );
    BOOST_CHECK( thrown);
}

void test_future_share() {
    // future retrieved from promise is valid (if it is the first)
    boost::fibers::promise< int > p1;
    int i = 7;
    p1.set_value( i);

    boost::fibers::future< int > f1 = p1.get_future();
    BOOST_CHECK( f1.valid() );

    // share
    boost::fibers::shared_future< int > sf1 = f1.share();
    BOOST_CHECK( sf1.valid() );
    BOOST_CHECK( ! f1.valid() );

    // get
    BOOST_CHECK( ! sf1.get_exception_ptr() );
    int j = sf1.get();
    BOOST_CHECK_EQUAL( i, j);
    BOOST_CHECK( sf1.valid() );
}

void test_future_share_ref() {
    // future retrieved from promise is valid (if it is the first)
    boost::fibers::promise< int& > p1;
    int i = 7;
    p1.set_value( i);

    boost::fibers::future< int& > f1 = p1.get_future();
    BOOST_CHECK( f1.valid() );

    // share
    boost::fibers::shared_future< int& > sf1 = f1.share();
    BOOST_CHECK( sf1.valid() );
    BOOST_CHECK( ! f1.valid() );

    // get
    BOOST_CHECK( ! sf1.get_exception_ptr() );
    int & j = sf1.get();
    BOOST_CHECK( &i == &j);
    BOOST_CHECK( sf1.valid() );
}

void test_future_share_void() {
    // future retrieved from promise is valid (if it is the first)
    boost::fibers::promise< void > p1;
    p1.set_value();

    boost::fibers::future< void > f1 = p1.get_future();
    BOOST_CHECK( f1.valid() );

    // share
    boost::fibers::shared_future< void > sf1 = f1.share();
    BOOST_CHECK( sf1.valid() );
    BOOST_CHECK( ! f1.valid() );

    // get
    BOOST_CHECK( ! sf1.get_exception_ptr() );
    sf1.get();
    BOOST_CHECK( sf1.valid() );
}

void test_future_wait() {
    // future retrieved from promise is valid (if it is the first)
    boost::fibers::promise< int > p1;
    boost::fibers::future< int > f1 = p1.get_future();

    // wait on future
    p1.set_value( 7);
    f1.wait();
    BOOST_CHECK( 7 == f1.get() );
}

void test_future_wait_ref() {
    // future retrieved from promise is valid (if it is the first)
    boost::fibers::promise< int& > p1;
    boost::fibers::future< int& > f1 = p1.get_future();

    // wait on future
    int i = 7;
    p1.set_value( i);
    f1.wait();
    int & j = f1.get();
    BOOST_CHECK( &i == &j);
}

void test_future_wait_void() {
    // future retrieved from promise is valid (if it is the first)
    boost::fibers::promise< void > p1;
    boost::fibers::future< void > f1 = p1.get_future();

    // wait on future
    p1.set_value();
    f1.wait();
    f1.get();
    BOOST_CHECK( ! f1.valid() );
}

void test_future_wait_with_fiber_1() {
    boost::fibers::promise< int > p1;
    boost::fibers::fiber( fn1, & p1, 7).detach();

    boost::fibers::future< int > f1 = p1.get_future();

    // wait on future
    BOOST_CHECK( 7 == f1.get() );
}

void test_future_wait_with_fiber_2() {
    boost::fibers::fiber( fn2).join();
}

void test_shared_future_move() {
    // future retrieved from promise is valid (if it is the first)
    boost::fibers::promise< int > p1;
    boost::fibers::shared_future< int > f1 = p1.get_future().share();
    BOOST_CHECK( f1.valid() );

    // move construction
    boost::fibers::shared_future< int > f2( std::move( f1) );
    BOOST_CHECK( ! f1.valid() );
    BOOST_CHECK( f2.valid() );

    // move assignment
    f1 = std::move( f2);
    BOOST_CHECK( f1.valid() );
    BOOST_CHECK( ! f2.valid() );
}

void test_shared_future_move_move() {
    // future retrieved from promise is valid (if it is the first)
    boost::fibers::promise< A > p1;
    boost::fibers::shared_future< A > f1 = p1.get_future().share();
    BOOST_CHECK( f1.valid() );

    // move construction
    boost::fibers::shared_future< A > f2( std::move( f1) );
    BOOST_CHECK( ! f1.valid() );
    BOOST_CHECK( f2.valid() );

    // move assignment
    f1 = std::move( f2);
    BOOST_CHECK( f1.valid() );
    BOOST_CHECK( ! f2.valid() );
}

void test_shared_future_move_ref() {
    // future retrieved from promise is valid (if it is the first)
    boost::fibers::promise< int& > p1;
    boost::fibers::shared_future< int& > f1 = p1.get_future().share();
    BOOST_CHECK( f1.valid() );

    // move construction
    boost::fibers::shared_future< int& > f2( std::move( f1) );
    BOOST_CHECK( ! f1.valid() );
    BOOST_CHECK( f2.valid() );

    // move assignment
    f1 = std::move( f2);
    BOOST_CHECK( f1.valid() );
    BOOST_CHECK( ! f2.valid() );
}

void test_shared_future_move_void() {
    // future retrieved from promise is valid (if it is the first)
    boost::fibers::promise< void > p1;
    boost::fibers::shared_future< void > f1 = p1.get_future().share();
    BOOST_CHECK( f1.valid() );

    // move construction
    boost::fibers::shared_future< void > f2( std::move( f1) );
    BOOST_CHECK( ! f1.valid() );
    BOOST_CHECK( f2.valid() );

    // move assignment
    f1 = std::move( f2);
    BOOST_CHECK( f1.valid() );
    BOOST_CHECK( ! f2.valid() );
}

// packaged_task
void test_packaged_task_create() {
    // default constructed packaged_task is not valid
    boost::fibers::packaged_task< int() > t1;
    BOOST_CHECK( ! t1.valid() );

    // packaged_task from function
    boost::fibers::packaged_task< int() > t2( fn3);
    BOOST_CHECK( t2.valid() );
}

// packaged_task
void test_packaged_task_create_move() {
    // default constructed packaged_task is not valid
    boost::fibers::packaged_task< A() > t1;
    BOOST_CHECK( ! t1.valid() );

    // packaged_task from function
    boost::fibers::packaged_task< A() > t2( fn9);
    BOOST_CHECK( t2.valid() );
}

void test_packaged_task_create_void() {
    // default constructed packaged_task is not valid
    boost::fibers::packaged_task< void() > t1;
    BOOST_CHECK( ! t1.valid() );

    // packaged_task from function
    boost::fibers::packaged_task< void() > t2( fn4);
    BOOST_CHECK( t2.valid() );
}

void test_packaged_task_move() {
    boost::fibers::packaged_task< int() > t1( fn3);
    BOOST_CHECK( t1.valid() );

    // move construction
    boost::fibers::packaged_task< int() > t2( std::move( t1) );
    BOOST_CHECK( ! t1.valid() );
    BOOST_CHECK( t2.valid() );

    // move assignment
    t1 = std::move( t2);
    BOOST_CHECK( t1.valid() );
    BOOST_CHECK( ! t2.valid() );
}

void test_packaged_task_move_move() {
    boost::fibers::packaged_task< A() > t1( fn9);
    BOOST_CHECK( t1.valid() );

    // move construction
    boost::fibers::packaged_task< A() > t2( std::move( t1) );
    BOOST_CHECK( ! t1.valid() );
    BOOST_CHECK( t2.valid() );

    // move assignment
    t1 = std::move( t2);
    BOOST_CHECK( t1.valid() );
    BOOST_CHECK( ! t2.valid() );
}

void test_packaged_task_move_void() {
    boost::fibers::packaged_task< void() > t1( fn4);
    BOOST_CHECK( t1.valid() );

    // move construction
    boost::fibers::packaged_task< void() > t2( std::move( t1) );
    BOOST_CHECK( ! t1.valid() );
    BOOST_CHECK( t2.valid() );

    // move assignment
    t1 = std::move( t2);
    BOOST_CHECK( t1.valid() );
    BOOST_CHECK( ! t2.valid() );
}

void test_packaged_task_swap() {
    boost::fibers::packaged_task< int() > t1( fn3);
    BOOST_CHECK( t1.valid() );

    boost::fibers::packaged_task< int() > t2;
    BOOST_CHECK( ! t2.valid() );

    // swap
    t1.swap( t2);
    BOOST_CHECK( ! t1.valid() );
    BOOST_CHECK( t2.valid() );
}

void test_packaged_task_swap_move() {
    boost::fibers::packaged_task< A() > t1( fn9);
    BOOST_CHECK( t1.valid() );

    boost::fibers::packaged_task< A() > t2;
    BOOST_CHECK( ! t2.valid() );

    // swap
    t1.swap( t2);
    BOOST_CHECK( ! t1.valid() );
    BOOST_CHECK( t2.valid() );
}

void test_packaged_task_swap_void() {
    boost::fibers::packaged_task< void() > t1( fn4);
    BOOST_CHECK( t1.valid() );

    boost::fibers::packaged_task< void() > t2;
    BOOST_CHECK( ! t2.valid() );

    // swap
    t1.swap( t2);
    BOOST_CHECK( ! t1.valid() );
    BOOST_CHECK( t2.valid() );
}

void test_packaged_task_reset() {
    {
        boost::fibers::packaged_task< int() > p( fn3);
        boost::fibers::future< int > f( p.get_future() );
        BOOST_CHECK( p.valid() );

        p();
        BOOST_CHECK( 3 == f.get() );

        // reset
        p.reset();
        p();
        f = p.get_future();
        BOOST_CHECK( 3 == f.get() );
    }
    {
        boost::fibers::packaged_task< int() > p;

        bool thrown = false;
        try {
            p.reset();
        } catch ( boost::fibers::packaged_task_uninitialized const&) {
            thrown = true;
        }
        BOOST_CHECK( thrown);
    }
}

void test_packaged_task_reset_move() {
    {
        boost::fibers::packaged_task< A() > p( fn9);
        boost::fibers::future< A > f( p.get_future() );
        BOOST_CHECK( p.valid() );

        p();
        BOOST_CHECK( 3 == f.get().value);

        // reset
        p.reset();
        p();
        f = p.get_future();
        BOOST_CHECK( 3 == f.get().value);
    }
    {
        boost::fibers::packaged_task< A() > p;

        bool thrown = false;
        try {
            p.reset();
        } catch ( boost::fibers::packaged_task_uninitialized const&) {
            thrown = true;
        }
        BOOST_CHECK( thrown);
    }
}

void test_packaged_task_reset_void() {
    {
        boost::fibers::packaged_task< void() > p( fn4);
        boost::fibers::future< void > f( p.get_future() );
        BOOST_CHECK( p.valid() );

        p();
        f.get();

        // reset
        p.reset();
        p();
        f = p.get_future();
        f.get();
    }
    {
        boost::fibers::packaged_task< void() > p;

        bool thrown = false;
        try {
            p.reset();
        } catch ( boost::fibers::packaged_task_uninitialized const&) {
            thrown = true;
        }
        BOOST_CHECK( thrown);
    }
}

void test_packaged_task_get_future() {
    boost::fibers::packaged_task< int() > t1( fn3);
    BOOST_CHECK( t1.valid() );

    // retrieve future
    boost::fibers::future< int > f1 = t1.get_future();
    BOOST_CHECK( f1.valid() );

    // retrieve future a second time
    bool thrown = false;
    try {
        f1 = t1.get_future();
    } catch ( boost::fibers::future_already_retrieved const&) {
        thrown = true;
    }
    BOOST_CHECK( thrown);

    // move construction
    boost::fibers::packaged_task< int() > t2( std::move( t1) );
    BOOST_CHECK( ! t1.valid() );
    BOOST_CHECK( t2.valid() );

    // retrieve future from uninitialized
    thrown = false;
    try {
        f1 = t1.get_future();
    } catch ( boost::fibers::packaged_task_uninitialized const&) {
        thrown = true;
    }
    BOOST_CHECK( thrown);
}

void test_packaged_task_get_future_move() {
    boost::fibers::packaged_task< A() > t1( fn9);
    BOOST_CHECK( t1.valid() );

    // retrieve future
    boost::fibers::future< A > f1 = t1.get_future();
    BOOST_CHECK( f1.valid() );

    // retrieve future a second time
    bool thrown = false;
    try {
        f1 = t1.get_future();
    } catch ( boost::fibers::future_already_retrieved const&) {
        thrown = true;
    }
    BOOST_CHECK( thrown);

    // move construction
    boost::fibers::packaged_task< A() > t2( std::move( t1) );
    BOOST_CHECK( ! t1.valid() );
    BOOST_CHECK( t2.valid() );

    // retrieve future from uninitialized
    thrown = false;
    try {
        f1 = t1.get_future();
    } catch ( boost::fibers::packaged_task_uninitialized const&) {
        thrown = true;
    }
    BOOST_CHECK( thrown);
}

void test_packaged_task_get_future_void() {
    boost::fibers::packaged_task< void() > t1( fn4);
    BOOST_CHECK( t1.valid() );

    // retrieve future
    boost::fibers::future< void > f1 = t1.get_future();
    BOOST_CHECK( f1.valid() );

    // retrieve future a second time
    bool thrown = false;
    try {
        f1 = t1.get_future();
    } catch ( boost::fibers::future_already_retrieved const&) {
        thrown = true;
    }
    BOOST_CHECK( thrown);

    // move construction
    boost::fibers::packaged_task< void() > t2( std::move( t1) );
    BOOST_CHECK( ! t1.valid() );
    BOOST_CHECK( t2.valid() );

    // retrieve future from uninitialized
    thrown = false;
    try {
        f1 = t1.get_future();
    } catch ( boost::fibers::packaged_task_uninitialized const&) {
        thrown = true;
    }
    BOOST_CHECK( thrown);
}

void test_packaged_task_exec() {
    // promise takes a copyable as return type
    boost::fibers::packaged_task< int() > t1( fn3);
    BOOST_CHECK( t1.valid() );
    boost::fibers::future< int > f1 = t1.get_future();
    BOOST_CHECK( f1.valid() );

    // exec
    t1();
    BOOST_CHECK( 3 == f1.get() );

    // exec a second time
    bool thrown = false;
    try {
        t1();
    } catch ( boost::fibers::promise_already_satisfied const&) {
        thrown = true;
    }
    BOOST_CHECK( thrown);
}

void test_packaged_task_exec_move() {
    // promise takes a copyable as return type
    boost::fibers::packaged_task< A() > t1( fn9);
    BOOST_CHECK( t1.valid() );
    boost::fibers::future< A > f1 = t1.get_future();
    BOOST_CHECK( f1.valid() );

    // exec
    t1();
    BOOST_CHECK( 3 == f1.get().value);

    // exec a second time
    bool thrown = false;
    try {
        t1();
    } catch ( boost::fibers::promise_already_satisfied const&) {
        thrown = true;
    }
    BOOST_CHECK( thrown);
}

void test_packaged_task_exec_param() {
    // promise takes a copyable as return type
    boost::fibers::packaged_task< int( int) > t1( fn8);
    BOOST_CHECK( t1.valid() );
    boost::fibers::future< int > f1 = t1.get_future();
    BOOST_CHECK( f1.valid() );

    // exec
    t1( 3);
    BOOST_CHECK( 3 == f1.get() );

    // exec a second time
    bool thrown = false;
    try {
        t1( 7);
    } catch ( boost::fibers::promise_already_satisfied const&) {
        thrown = true;
    }
    BOOST_CHECK( thrown);

    //TODO: packaged_task returns a moveable-only as return type
}

void test_packaged_task_exec_ref() {
    // promise takes a copyable as return type
    boost::fibers::packaged_task< int&() > t1( fn7);
    BOOST_CHECK( t1.valid() );
    boost::fibers::future< int& > f1 = t1.get_future();
    BOOST_CHECK( f1.valid() );

    // exec
    t1();
    int & i = f1.get();
    BOOST_CHECK( &gi == &i);

    // exec a second time
    bool thrown = false;
    try {
        t1();
    } catch ( boost::fibers::promise_already_satisfied const&) {
        thrown = true;
    }
    BOOST_CHECK( thrown);

    //TODO: packaged_task returns a moveable-only as return type
}

void test_packaged_task_exec_void() {
    // promise takes a copyable as return type
    boost::fibers::packaged_task< void() > t1( fn4);
    BOOST_CHECK( t1.valid() );
    boost::fibers::future< void > f1 = t1.get_future();
    BOOST_CHECK( f1.valid() );

    // set void
    t1();
    f1.get();

    // exec a second time
    bool thrown = false;
    try {
        t1();
    } catch ( boost::fibers::promise_already_satisfied const&) {
        thrown = true;
    }
    BOOST_CHECK( thrown);
}

void test_packaged_task_exception() {
    // promise takes a copyable as return type
    boost::fibers::packaged_task< int() > t1( fn5);
    BOOST_CHECK( t1.valid() );
    boost::fibers::future< int > f1 = t1.get_future();
    BOOST_CHECK( f1.valid() );

    // exec
    t1();
    bool thrown = false;
    try {
        f1.get();
    } catch ( my_exception const&) {
        thrown = true;
    }
    BOOST_CHECK( thrown);

    boost::fibers::packaged_task< int() > t2( fn5);
    BOOST_CHECK( t2.valid() );
    boost::fibers::future< int > f2 = t2.get_future();
    BOOST_CHECK( f2.valid() );

    // exec
    t2();
    BOOST_CHECK( f2.get_exception_ptr() );
    thrown = false;
    try
    { std::rethrow_exception( f2.get_exception_ptr() ); }
    catch ( my_exception const&)
    { thrown = true; }
    BOOST_CHECK( thrown);
}

void test_packaged_task_exception_move() {
    // promise takes a moveable as return type
    boost::fibers::packaged_task< A() > t1( fn10);
    BOOST_CHECK( t1.valid() );
    boost::fibers::future< A > f1 = t1.get_future();
    BOOST_CHECK( f1.valid() );

    // exec
    t1();
    bool thrown = false;
    try {
        f1.get();
    } catch ( my_exception const&) {
        thrown = true;
    }
    BOOST_CHECK( thrown);

    boost::fibers::packaged_task< A() > t2( fn10);
    BOOST_CHECK( t2.valid() );
    boost::fibers::future< A > f2 = t2.get_future();
    BOOST_CHECK( f2.valid() );

    // exec
    t2();
    BOOST_CHECK( f2.get_exception_ptr() );
    thrown = false;
    try
    { std::rethrow_exception( f2.get_exception_ptr() ); }
    catch ( my_exception const&)
    { thrown = true; }
    BOOST_CHECK( thrown);
}

void test_packaged_task_exception_void() {
    // promise takes a copyable as return type
    boost::fibers::packaged_task< void() > t1( fn6);
    BOOST_CHECK( t1.valid() );
    boost::fibers::future< void > f1 = t1.get_future();
    BOOST_CHECK( f1.valid() );

    // set void
    t1();
    bool thrown = false;
    try {
        f1.get();
    } catch ( my_exception const&) {
        thrown = true;
    }
    BOOST_CHECK( thrown);
    
    boost::fibers::packaged_task< void() > t2( fn6);
    BOOST_CHECK( t2.valid() );
    boost::fibers::future< void > f2 = t2.get_future();
    BOOST_CHECK( f2.valid() );

    // exec
    t2();
    BOOST_CHECK( f2.get_exception_ptr() );
    thrown = false;
    try {
        std::rethrow_exception( f2.get_exception_ptr() );
    } catch ( my_exception const&) {
        thrown = true;
    }
    BOOST_CHECK( thrown);
}

void test_async_1() {
    boost::fibers::future< int > f1 = boost::fibers::async( fn3);
    BOOST_CHECK( f1.valid() );

    BOOST_CHECK( 3 == f1.get());
}

void test_async_2() {
    boost::fibers::future< int > f1 = boost::fibers::async( fn8, 3);
    BOOST_CHECK( f1.valid() );

    BOOST_CHECK( 3 == f1.get());
}


boost::unit_test_framework::test_suite* init_unit_test_suite(int, char*[]) {
    boost::unit_test_framework::test_suite* test =
        BOOST_TEST_SUITE("Boost.Fiber: futures test suite");

    test->add(BOOST_TEST_CASE(test_promise_create));
    test->add(BOOST_TEST_CASE(test_promise_create_ref));
    test->add(BOOST_TEST_CASE(test_promise_create_void));
    test->add(BOOST_TEST_CASE(test_promise_move));
    test->add(BOOST_TEST_CASE(test_promise_move_ref));
    test->add(BOOST_TEST_CASE(test_promise_move_void));
    test->add(BOOST_TEST_CASE(test_promise_swap));
    test->add(BOOST_TEST_CASE(test_promise_swap_ref));
    test->add(BOOST_TEST_CASE(test_promise_swap_void));
    test->add(BOOST_TEST_CASE(test_promise_get_future));
    test->add(BOOST_TEST_CASE(test_promise_get_future_ref));
    test->add(BOOST_TEST_CASE(test_promise_get_future_void));
    test->add(BOOST_TEST_CASE(test_promise_set_value));
    test->add(BOOST_TEST_CASE(test_promise_set_value_move));
    test->add(BOOST_TEST_CASE(test_promise_set_value_ref));
    test->add(BOOST_TEST_CASE(test_promise_set_value_void));
    test->add(BOOST_TEST_CASE(test_promise_set_exception));
    test->add(BOOST_TEST_CASE(test_promise_set_exception_ref));
    test->add(BOOST_TEST_CASE(test_promise_set_exception_void));

    test->add(BOOST_TEST_CASE(test_future_create));
    test->add(BOOST_TEST_CASE(test_future_create_ref));
    test->add(BOOST_TEST_CASE(test_future_create_void));
    test->add(BOOST_TEST_CASE(test_future_move));
    test->add(BOOST_TEST_CASE(test_future_move_ref));
    test->add(BOOST_TEST_CASE(test_future_move_void));
    test->add(BOOST_TEST_CASE(test_future_get));
    test->add(BOOST_TEST_CASE(test_future_get_move));
    test->add(BOOST_TEST_CASE(test_future_get_ref));
    test->add(BOOST_TEST_CASE(test_future_get_void));
    test->add(BOOST_TEST_CASE(test_future_share));
    test->add(BOOST_TEST_CASE(test_future_share_ref));
    test->add(BOOST_TEST_CASE(test_future_share_void));
    test->add(BOOST_TEST_CASE(test_future_wait));
    test->add(BOOST_TEST_CASE(test_future_wait_ref));
    test->add(BOOST_TEST_CASE(test_future_wait_void));
    test->add(BOOST_TEST_CASE(test_future_wait_with_fiber_1));
    test->add(BOOST_TEST_CASE(test_future_wait_with_fiber_2));

    test->add(BOOST_TEST_CASE(test_shared_future_move));
    test->add(BOOST_TEST_CASE(test_shared_future_move_move));
    test->add(BOOST_TEST_CASE(test_shared_future_move_ref));
    test->add(BOOST_TEST_CASE(test_shared_future_move_void));

    test->add(BOOST_TEST_CASE(test_packaged_task_create));
    test->add(BOOST_TEST_CASE(test_packaged_task_create_move));
    test->add(BOOST_TEST_CASE(test_packaged_task_create_void));
    test->add(BOOST_TEST_CASE(test_packaged_task_move));
    test->add(BOOST_TEST_CASE(test_packaged_task_move_move));
    test->add(BOOST_TEST_CASE(test_packaged_task_move_void));
    test->add(BOOST_TEST_CASE(test_packaged_task_swap));
    test->add(BOOST_TEST_CASE(test_packaged_task_swap_move));
    test->add(BOOST_TEST_CASE(test_packaged_task_swap_void));
    test->add(BOOST_TEST_CASE(test_packaged_task_reset));
    test->add(BOOST_TEST_CASE(test_packaged_task_reset_move));
    test->add(BOOST_TEST_CASE(test_packaged_task_reset_void));
    test->add(BOOST_TEST_CASE(test_packaged_task_get_future));
    test->add(BOOST_TEST_CASE(test_packaged_task_get_future_move));
    test->add(BOOST_TEST_CASE(test_packaged_task_get_future_void));
    test->add(BOOST_TEST_CASE(test_packaged_task_exec));
    test->add(BOOST_TEST_CASE(test_packaged_task_exec_move));
    test->add(BOOST_TEST_CASE(test_packaged_task_exec_param));
    test->add(BOOST_TEST_CASE(test_packaged_task_exec_ref));
    test->add(BOOST_TEST_CASE(test_packaged_task_exec_void));
    test->add(BOOST_TEST_CASE(test_packaged_task_exception));
    test->add(BOOST_TEST_CASE(test_packaged_task_exception_move));
    test->add(BOOST_TEST_CASE(test_packaged_task_exception_void));

    test->add(BOOST_TEST_CASE(test_async_1));
    test->add(BOOST_TEST_CASE(test_async_2));

    return test;
}
