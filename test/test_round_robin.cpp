
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cstdio>
#include <string>

#include <boost/atomic.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/thread.hpp>
#include <boost/utility.hpp>

#include <boost/fiber/all.hpp>
#include <boost/fiber/detail/scheduler.hpp>

#define MAXCOUNT 50

boost::atomic< bool > fini( false);
boost::fibers::round_robin * other_ds = 0;
boost::fibers::fiber * other_f = 0;

int lazy_generate( boost::barrier * b, int * value, boost::fibers::condition * c)
{
    * value = 1;
    boost::this_fiber::yield();

    if ( b) b->wait();

    * value = 2;
    boost::this_fiber::yield();
    boost::this_fiber::interruption_point();
    * value = 3;
    boost::this_fiber::yield();
    boost::this_fiber::interruption_point();
    * value = 4;
    boost::this_fiber::yield();
    boost::this_fiber::interruption_point();
    * value = 5;
    boost::this_fiber::yield();
    boost::this_fiber::interruption_point();
    * value = 6;

    if ( c) c->notify_all();

    return * value;
}

void join_fiber( boost::barrier * b, int * value, bool * interrupted)
{
    b->wait();
    try
    {
        other_f->join();
        * value = 7;
    }
    catch ( boost::fibers::fiber_interrupted const&)
    { * interrupted = true; }
}

void interrupt_join_fiber( boost::barrier * b, int * value, bool * interrupted)
{
    b->wait();
    other_f->interrupt();
    try
    {
        other_f->join();
        * value = 7;
    }
    catch ( boost::fibers::fiber_interrupted const&)
    { * interrupted = true; }
}

void running_future( boost::barrier * b, int * value)
{
    boost::fibers::packaged_task<void> pt(
            boost::bind( lazy_generate, b, value, ( boost::fibers::condition *) 0) );
    boost::fibers::unique_future<void> ft = pt.get_future();
    other_f = new boost::fibers::fiber( boost::move( pt) );
    ft.wait();
}

void running_condition( boost::barrier * b, int * value)
{
    boost::fibers::mutex m;
    boost::fibers::condition c;
    other_f = new boost::fibers::fiber(
            boost::bind( lazy_generate, b, value, & c) );
    boost::fibers::mutex::scoped_lock lk( m);
    c.wait( lk);
}

void terminated( boost::barrier * b, int * value)
{
    boost::fibers::packaged_task<void> pt(
            boost::bind( lazy_generate, ( boost::barrier *) 0, value, ( boost::fibers::condition *) 0) );
    boost::fibers::unique_future<void> ft = pt.get_future();
    other_f = new boost::fibers::fiber( boost::move( pt) );
    ft.wait();
    b->wait();
}

void interrupt_from_same_thread( boost::barrier * b, int * value, bool * interrupted)
{
    boost::fibers::packaged_task<int> pt( boost::bind( lazy_generate, b, value, ( boost::fibers::condition *) 0) );
    boost::fibers::unique_future<int> ft = pt.get_future();
    other_f = new boost::fibers::fiber( boost::move( pt) );
    other_f->interrupt();
    // other_f will joined by another fiber
    try
    {
        int res = ft.get();
        BOOST_ASSERT( res > 0);
    }
    catch ( boost::fibers::fiber_interrupted const&)
    { * interrupted = true; }
}

void interrupt_from_other_thread( boost::barrier * b, int * value, bool * result)
{
    boost::fibers::packaged_task<void> pt( boost::bind( lazy_generate, b, value, ( boost::fibers::condition *) 0) );
    boost::fibers::unique_future<void> ft = pt.get_future();
    other_f = new boost::fibers::fiber( boost::move( pt) );
    // other_f will joined by another fiber
    try
    { ft.get(); }
    catch ( boost::fibers::fiber_interrupted const&)
    {}
    * result = true;
}

void fn_running_future( boost::barrier * b, int * value)
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    boost::fibers::fiber(
        boost::bind( running_future, b, value) ).join();
}

void fn_running_condition( boost::barrier * b, int * value)
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    boost::fibers::fiber(
        boost::bind( running_condition, b, value) ).join();
}

void fn_terminated( boost::barrier * b, int * value)
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    boost::fibers::fiber(
        boost::bind( terminated, b, value) ).join();
}

void fn_interrupt_from_same_thread( boost::barrier * b, int * value, bool * interrupted)
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    boost::fibers::fiber f(
        boost::bind( interrupt_from_same_thread, b, value, interrupted) );

    f.join();
}

void fn_interrupt_from_other_thread( boost::barrier * b, int * value, bool * result)
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    boost::fibers::fiber(
        boost::bind( interrupt_from_other_thread, b, value, result) ).join();
}

void fn_join_in_fiber( boost::barrier * b, int * value, bool * interrupted)
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    boost::fibers::fiber f(
        boost::bind( join_fiber, b, value, interrupted) );

    f.join();
}

void fn_join_in_fiber_interrupt( boost::barrier * b, int * value, bool * interrupted)
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    boost::fibers::fiber(
        boost::bind( interrupt_join_fiber, b, value, interrupted) ).join();
}

void test_join_in_fiber_runing_future()
{
    for ( int i = 0; i < MAXCOUNT; ++i) {
    int value1 = 0;
    int value2 = 0;
    bool interrupted = false;

    boost::barrier b( 2);
    boost::thread t1( boost::bind( fn_running_future, &b, &value1) );
    boost::thread t2( boost::bind( fn_join_in_fiber, &b, &value2, &interrupted) );

    t1.join();
    t2.join();

    BOOST_CHECK_EQUAL( 6, value1);
    BOOST_CHECK_EQUAL( 7, value2);
    BOOST_CHECK( ! interrupted);
    delete other_f;
    fprintf(stderr, "%s finished\n", __func__);
    }
}

void test_join_in_fiber_runing_condition()
{
    for ( int i = 0; i < MAXCOUNT; ++i) {
    int value1 = 0;
    int value2 = 0;
    bool interrupted = false;

    boost::barrier b( 2);
    boost::thread t1( boost::bind( fn_running_condition, &b, &value1) );
    boost::thread t2( boost::bind( fn_join_in_fiber, &b, &value2, &interrupted) );

    t1.join();
    t2.join();

    BOOST_CHECK_EQUAL( 6, value1);
    BOOST_CHECK_EQUAL( 7, value2);
    BOOST_CHECK( ! interrupted);
    delete other_f;
    fprintf(stderr, "%s finished\n", __func__);
    }
}

void test_join_in_fiber_terminated()
{
    for ( int i = 0; i < MAXCOUNT; ++i) {
    int value1 = 0;
    int value2 = 0;
    bool interrupted = false;

    boost::barrier b( 2);
    boost::thread t1( boost::bind( fn_terminated, &b, &value1) );
    boost::thread t2( boost::bind( fn_join_in_fiber, &b, &value2, &interrupted) );

    t1.join();
    t2.join();

    BOOST_CHECK_EQUAL( 6, value1);
    BOOST_CHECK_EQUAL( 7, value2);
    BOOST_CHECK( ! interrupted);
    delete other_f;
    fprintf(stderr, "%s finished\n", __func__);
    }
}

void test_join_in_fiber_interrupted_inside()
{
    for ( int i = 0; i < MAXCOUNT; ++i) {
    int value1 = 0;
    int value2 = 0;
    bool interrupted1 = false, interrupted2 = false;

    boost::barrier b( 2);
    boost::thread t1( boost::bind( fn_interrupt_from_same_thread, &b, &value1, &interrupted1) );
    boost::thread t2( boost::bind( fn_join_in_fiber, &b, &value2, &interrupted2) );

    t1.join();
    t2.join();

    BOOST_CHECK( interrupted1);
    BOOST_CHECK( !interrupted2);
    BOOST_CHECK_EQUAL( 7, value2);
    delete other_f;
    fprintf(stderr, "%s finished\n", __func__);
    }
}

void test_join_in_fiber_interrupted_outside()
{
    for ( int i = 0; i < MAXCOUNT; ++i) {
    int value1 = 0;
    int value2 = 0;
    bool result = false, interrupted = false;

    boost::barrier b( 2);
    boost::thread t1( boost::bind( fn_interrupt_from_other_thread, &b, &value1, &result) );
    boost::thread t2( boost::bind( fn_join_in_fiber_interrupt, &b, &value2, &interrupted) );

    t1.join();
    t2.join();

    BOOST_CHECK( result);
    BOOST_CHECK( ! interrupted);
    BOOST_CHECK_EQUAL( 7, value2);
    delete other_f;
    fprintf(stderr, "%s finished\n", __func__);
    }
}

void mutex_exclusive_1( boost::fibers::mutex * mtx, int * value)
{
	boost::fibers::mutex::scoped_lock lk( * mtx);
	++( * value);
	for ( int i = 0; i < 3; ++i)
		boost::this_fiber::yield();
}

void mutex_exclusive_2( boost::fibers::mutex * mtx, int * value)
{
	++( * value);
	boost::fibers::mutex::scoped_lock lk( * mtx);
	++( * value);
}

void fn_mutex_exclusive_1( boost::fibers::mutex * mtx, int * value)
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    boost::fibers::fiber(
		boost::bind( & mutex_exclusive_1, mtx, value) ).join();
}

void fn_mutex_exclusive_2( boost::fibers::mutex * mtx, int * value)
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    boost::fibers::fiber(
		boost::bind( & mutex_exclusive_2, mtx, value) ).join();
}

void test_mutex_exclusive()
{
    for ( int i = 0; i < MAXCOUNT; ++i) {
    int value1 = 0;
    int value2 = 0;

    boost::fibers::mutex mtx;
    boost::thread t1( boost::bind( fn_mutex_exclusive_1, &mtx, &value1) );
    boost::thread t2( boost::bind( fn_mutex_exclusive_2, &mtx, &value2) );

    t1.join();
    t2.join();

	BOOST_CHECK_EQUAL( 1, value1);
	BOOST_CHECK_EQUAL( 2, value2);
    fprintf(stderr, "%s finished\n", __func__);
    }
}

void wait_fn( boost::fibers::mutex * mtx, boost::fibers::condition * cond,
              int * value)
{
	boost::fibers::mutex::scoped_lock lk( * mtx);
	cond->wait( lk);
	++( * value);
}

void fn_two_waiter( boost::barrier * b, boost::fibers::mutex * mtx,
                    boost::fibers::condition * cond, int * value)
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    boost::fibers::fiber s1(
            boost::bind( wait_fn, mtx, cond, value) );
	BOOST_CHECK_EQUAL( 0, * value);

    boost::fibers::fiber s2(
            boost::bind( wait_fn, mtx, cond, value) );
	BOOST_CHECK_EQUAL( 0, * value);

    b->wait();

    s1.join();
    s2.join();

	BOOST_CHECK_EQUAL( 2, * value);
}

void notify_one_fn( boost::fibers::condition * cond, int count)
{
    for ( int i = 0; i < count; ++i)
	    cond->notify_one();
}

void fn_notify_one( boost::barrier * b, boost::fibers::condition * cond)
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    b->wait();

    boost::fibers::fiber(
            boost::bind( notify_one_fn, cond, 2) ).join();
}

void test_two_waiter_notify_one()
{
    for ( int i = 0; i < MAXCOUNT; ++i) {
	int value = 0;

    boost::barrier b( 2);
	boost::fibers::mutex mtx;
	boost::fibers::condition cond;
    boost::thread t1( boost::bind( fn_two_waiter, &b, &mtx, &cond, &value) );
    boost::thread t2( boost::bind( fn_notify_one, &b, &cond) );

    t1.join();
    t2.join();

	BOOST_CHECK_EQUAL( 2, value);
    fprintf(stderr, "%s finished\n", __func__);
    }
}

void notify_all_fn( boost::fibers::condition * cond)
{
	cond->notify_all();
}

void fn_notify_all( boost::barrier * b, boost::fibers::condition * cond)
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    b->wait();

    boost::fibers::fiber(
            boost::bind( notify_all_fn, cond) ).join();
}

void test_two_waiter_notify_all()
{
    for ( int i = 0; i < MAXCOUNT; ++i) {
	int value = 0;

    boost::barrier b( 2);
	boost::fibers::mutex mtx;
	boost::fibers::condition cond;
    boost::thread t1( boost::bind( fn_two_waiter, &b, &mtx, &cond, &value) );
    boost::thread t2( boost::bind( fn_notify_all, &b, &cond) );

    t1.join();
    t2.join();

	BOOST_CHECK_EQUAL( 2, value);
    fprintf(stderr, "%s finished\n", __func__);
    }
}

boost::unit_test::test_suite * init_unit_test_suite( int, char* [])
{
    boost::unit_test::test_suite * test =
        BOOST_TEST_SUITE("Boost.Fiber: round_robin test suite");

//    test->add( BOOST_TEST_CASE( & test_join_in_fiber_runing_future) );
    test->add( BOOST_TEST_CASE( & test_join_in_fiber_runing_condition) );
#if 0
    test->add( BOOST_TEST_CASE( & test_join_in_fiber_terminated) );
    test->add( BOOST_TEST_CASE( & test_join_in_fiber_interrupted_inside) );
    test->add( BOOST_TEST_CASE( & test_join_in_fiber_interrupted_outside) );
    test->add( BOOST_TEST_CASE( & test_mutex_exclusive) );
    test->add( BOOST_TEST_CASE( & test_two_waiter_notify_one) );
    test->add( BOOST_TEST_CASE( & test_two_waiter_notify_all) );
#endif
    return test;
}
