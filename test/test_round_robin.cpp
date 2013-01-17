
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cstdio>
#include <sstream>
#include <string>

#include <boost/atomic.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/thread.hpp>
#include <boost/utility.hpp>

#include <boost/fiber/all.hpp>

#define MAXCOUNT 50

boost::atomic< bool > fini( false);
boost::fibers::round_robin * other_ds = 0;
boost::fibers::fiber * other_f = 0;

void lazy_generate( boost::barrier * b, int * value)
{
    * value = 1;
    boost::this_fiber::yield();

    if ( b) b->wait();

    boost::xtime xt;
    boost::xtime_get(&xt, boost::TIME_UTC_);
    xt.nsec += 50000000 ; // 50ms
    //xt.sec += 1; //1 second
    boost::this_thread::sleep( xt);

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
    boost::xtime xt;
    boost::xtime_get(&xt, boost::TIME_UTC_);
    xt.nsec += 150000000 ; // 50ms
    //xt.sec += 1; //1 second
    boost::this_thread::sleep( xt);
        other_f->join();
        * value = 7;
    }
    catch ( boost::fibers::fiber_interrupted const&)
    { * interrupted = true; }
}

void running( boost::barrier * b, int * value)
{
    boost::fibers::packaged_task<void> pt(
            boost::bind( lazy_generate, b, value) );
    boost::fibers::unique_future<void> ft = pt.get_future();
    other_f = new boost::fibers::fiber( boost::move( pt) );
    ft.wait();
}

void terminated( boost::barrier * b, int * value)
{
    boost::fibers::packaged_task<void> pt(
            boost::bind( lazy_generate, ( boost::barrier *) 0, value) );
    boost::fibers::unique_future<void> ft = pt.get_future();
    other_f = new boost::fibers::fiber( boost::move( pt) );
    ft.wait();
    b->wait();
}

void interrupt_from_same_thread( boost::barrier * b, int * value, bool * interrupted)
{
    boost::fibers::packaged_task<void> pt( boost::bind( lazy_generate, b, value) );
    boost::fibers::unique_future<void> ft = pt.get_future();
    other_f = new boost::fibers::fiber( boost::move( pt) );
    other_f->interrupt();
    // other_f will joined by another fiber
    try
    { ft.get(); }
    catch ( boost::fibers::fiber_interrupted const&)
    { * interrupted = true; }
}

void interrupt_from_other_thread( boost::barrier * b, int * value, bool * result)
{
    boost::fibers::packaged_task<void> pt( boost::bind( lazy_generate, b, value) );
    boost::fibers::unique_future<void> ft = pt.get_future();
    other_f = new boost::fibers::fiber( boost::move( pt) );
    // other_f will joined by another fiber
    try
    { ft.get(); }
    catch ( boost::fibers::fiber_interrupted const&)
    {}
    * result = true;
}

void fn_running( boost::barrier * b, int * value)
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    boost::fibers::fiber(
        boost::bind( running, b, value) ).join();
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

    boost::fibers::fiber(
        boost::bind( interrupt_from_same_thread, b, value, interrupted) ).join();
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

    boost::fibers::fiber(
        boost::bind( join_fiber, b, value, interrupted) ).join();
}

void fn_join_in_fiber_interrupt( boost::barrier * b, int * value, bool * interrupted)
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    boost::fibers::fiber(
        boost::bind( interrupt_join_fiber, b, value, interrupted) ).join();
}

void test_join_in_fiber_runing()
{
    for ( int i = 0; i < MAXCOUNT; ++i) {
    int value1 = 0;
    int value2 = 0;
    bool interrupted = false;

    boost::barrier b( 2);
    boost::thread t1( boost::bind( fn_running, &b, &value1) );
    boost::thread t2( boost::bind( fn_join_in_fiber, &b, &value2, &interrupted) );

    t1.join();
    t2.join();

    BOOST_CHECK_EQUAL( 6, value1);
    BOOST_CHECK_EQUAL( 7, value2);
    BOOST_CHECK( ! interrupted);
    delete other_f;
    fprintf(stderr, "%d. finished\n", i);
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
    fprintf(stderr, "%d. finished\n", i);
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
    fprintf(stderr, "%d. finished\n", i);
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
    fprintf(stderr, "%d. finished\n", i);
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
    fprintf(stderr, "%d. finished\n", i);
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
    fprintf(stderr, "%d. finished\n", i);
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
    fprintf(stderr, "%d. finished\n", i);
    }
}

boost::fibers::shared_future< int > fibonacci( int n);

int fibonacci_( int n)
{
    boost::this_fiber::yield();

    int res = 1;

    if ( 0 != n && 1 != n)
    {
        boost::fibers::shared_future< int > f1 = fibonacci( n - 1);
        boost::fibers::shared_future< int > f2 = fibonacci( n - 2);

        res = f1.get() + f2.get();
    }

    return res;
}

boost::fibers::shared_future< int > fibonacci( int n)
{
    boost::fibers::packaged_task<int> pt(
        boost::bind( fibonacci_, n) );
    boost::fibers::shared_future<int> f(pt.get_future());
    boost::fibers::fiber( boost::move(pt) ).detach();
    return f;
}

void create_fibers( int n)
{
    int res = fibonacci( n).get();

    fprintf(stderr, "fibonacci(%d) == %d\n", n, res);
}

void fn_create_fibers( boost::fibers::round_robin * ds, boost::barrier * b, int n)
{
    boost::fibers::scheduling_algorithm( ds);

    b->wait();

    boost::fibers::fiber f1(
        boost::bind( create_fibers, n) );
    boost::fibers::fiber f2(
        boost::bind( create_fibers, n) );

    f1.join();
    f2.join();

    fini = true;
}

void fn_steel_fibers( boost::fibers::round_robin * other_ds, boost::barrier * b, int * count)
{
    BOOST_ASSERT( other_ds);
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    b->wait();

    while ( ! fini)
    {
        boost::fibers::fiber f( other_ds->steel_from() );
        if ( f)
        {
            ++( * count);
            ds.migrate_to( f);
            while ( boost::fibers::run() );
        }
        f.detach();
    }
}

void test_migrate_fiber()
{
    for ( int i = 0; i < MAXCOUNT; ++i) {
    fini = false;
	int n = 10, count = 0;

    boost::fibers::round_robin * ds = new boost::fibers::round_robin();
    boost::barrier b( 2);
    boost::thread t1( boost::bind( fn_create_fibers, ds, &b, n) );
    boost::thread t2( boost::bind( fn_steel_fibers, ds, &b, &count) );

    t1.join();
    t2.join();

    fprintf(stderr, "stolen fibers == %d\n", count);
    fprintf(stderr, "%d. finished\n", i);
    delete ds;
    }
}

boost::unit_test::test_suite * init_unit_test_suite( int, char* [])
{
    boost::unit_test::test_suite * test =
        BOOST_TEST_SUITE("Boost.Fiber: round_robin test suite");
#if 0
    test->add( BOOST_TEST_CASE( & test_join_in_fiber_runing) );
    test->add( BOOST_TEST_CASE( & test_join_in_fiber_terminated) );
    test->add( BOOST_TEST_CASE( & test_join_in_fiber_interrupted_inside) );
    test->add( BOOST_TEST_CASE( & test_join_in_fiber_interrupted_outside) );
    test->add( BOOST_TEST_CASE( & test_mutex_exclusive) );
    test->add( BOOST_TEST_CASE( & test_two_waiter_notify_one) );
    test->add( BOOST_TEST_CASE( & test_two_waiter_notify_all) );
#endif
    test->add( BOOST_TEST_CASE( & test_migrate_fiber) );

    return test;
}
