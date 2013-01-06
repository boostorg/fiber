
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <map>
#include <stdexcept>
#include <vector>

#include <boost/bind.hpp>
#include <boost/chrono/system_clocks.hpp>
#include <boost/cstdint.hpp>
#include <boost/function.hpp>
#include <boost/ref.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/utility.hpp>

#include <boost/fiber/all.hpp>

int value = 0;

inline
boost::chrono::system_clock::time_point delay(int secs, int msecs=0, int nsecs=0)
{
    boost::chrono::system_clock::time_point t = boost::chrono::system_clock::now();
    t += boost::chrono::seconds( secs);
    t += boost::chrono::milliseconds( msecs);
    //t += boost::chrono::nanoseconds( nsecs);

    return t;
}

struct condition_test_data
{
    condition_test_data() : notified(0), awoken(0) { }

    boost::fibers::mutex mutex;
    boost::fibers::condition condition;
    int notified;
    int awoken;
};

struct cond_predicate
{
    cond_predicate(int& var, int val) : _var(var), _val(val) { }

    bool operator()() { return _var == _val; }

    int& _var;
    int _val;
private:
    void operator=(cond_predicate&);
    
};

void notify_one_fn( boost::fibers::condition & cond)
{
	cond.notify_one();
}

void notify_all_fn( boost::fibers::condition & cond)
{
	cond.notify_all();
}

void wait_fn(
	boost::fibers::mutex & mtx,
	boost::fibers::condition & cond)
{
	boost::fibers::mutex::scoped_lock lk( mtx);
	cond.wait( lk);
	++value;
}

void condition_test_waits( condition_test_data * data)
{
    boost::fibers::mutex::scoped_lock lock( data->mutex);
    BOOST_CHECK( lock ? true : false);

    // Test wait.
    fprintf( stderr, "fiber wait.\n");
    while ( data->notified != 1)
        data->condition.wait(lock);
    BOOST_CHECK(lock ? true : false);
    BOOST_CHECK_EQUAL(data->notified, 1);
    data->awoken++;
    data->condition.notify_one();

    // Test predicate wait.
    fprintf( stderr, "fiber predicate wait.\n");
    data->condition.wait(lock, cond_predicate(data->notified, 2));
    BOOST_CHECK(lock ? true : false);
    BOOST_CHECK_EQUAL(data->notified, 2);
    data->awoken++;
    data->condition.notify_one();

    // Test timed_wait.
    fprintf( stderr, "fiber timed_wait.\n");
    boost::chrono::system_clock::time_point xt = delay(10);
    while (data->notified != 3)
        data->condition.wait(lock);
        //data->condition.timed_wait(lock, xt);
    BOOST_CHECK(lock ? true : false);
    BOOST_CHECK_EQUAL(data->notified, 3);
    data->awoken++;
    data->condition.notify_one();

    // Test predicate timed_wait.
    fprintf( stderr, "fiber predicate timed_wait.\n");
    xt = delay(10);
    cond_predicate pred(data->notified, 4);
    BOOST_CHECK(data->condition.timed_wait(lock, xt, pred));
    BOOST_CHECK(lock ? true : false);
    BOOST_CHECK(pred());
    BOOST_CHECK_EQUAL(data->notified, 4);
    data->awoken++;
    data->condition.notify_one();

    // Test predicate timed_wait with relative timeout
    fprintf( stderr, "fiber predicate timed_wait with relative timeout.\n");
    cond_predicate pred_rel(data->notified, 5);
    BOOST_CHECK(data->condition.timed_wait(lock, boost::chrono::seconds(10), pred_rel));
    BOOST_CHECK(lock ? true : false);
    BOOST_CHECK(pred_rel());
    BOOST_CHECK_EQUAL(data->notified, 5);
    data->awoken++;
    data->condition.notify_one();

    // Test timeout timed_wait.
    fprintf( stderr, "fiber timeout timed_wait.\n");
    BOOST_CHECK(!data->condition.timed_wait(lock, boost::chrono::seconds(2)));
    BOOST_CHECK(lock ? true : false);
}

void do_test_condition_waits()
{
    condition_test_data data;

    boost::fibers::fiber s(
            boost::bind( & condition_test_waits, & data) );

    {
        boost::fibers::mutex::scoped_lock lock( data.mutex);
        BOOST_CHECK(lock ? true : false);

        data.notified++;
        data.condition.notify_one();
        while (data.awoken != 1)
            data.condition.wait(lock);
        BOOST_CHECK(lock ? true : false);
        BOOST_CHECK_EQUAL(data.awoken, 1);

        data.notified++;
        data.condition.notify_one();
        while (data.awoken != 2)
            data.condition.wait(lock);
        BOOST_CHECK(lock ? true : false);
        BOOST_CHECK_EQUAL(data.awoken, 2);

        data.notified++;
        data.condition.notify_one();
        while (data.awoken != 3)
            data.condition.wait(lock);
        BOOST_CHECK(lock ? true : false);
        BOOST_CHECK_EQUAL(data.awoken, 3);

        data.notified++;
        data.condition.notify_one();
        while (data.awoken != 4)
            data.condition.wait(lock);
        BOOST_CHECK(lock ? true : false);
        BOOST_CHECK_EQUAL(data.awoken, 4);

        data.notified++;
        data.condition.notify_one();
        while (data.awoken != 5)
            data.condition.wait(lock);
        BOOST_CHECK(lock ? true : false);
        BOOST_CHECK_EQUAL(data.awoken, 5);
    }

    if ( s.joinable() ) s.join();
    BOOST_CHECK_EQUAL(data.awoken, 5);
}

void test_one_waiter_notify_one()
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

	value = 0;
	boost::fibers::mutex mtx;
	boost::fibers::condition cond;

    boost::fibers::fiber s1(
            boost::bind(
                wait_fn,
                boost::ref( mtx),
                boost::ref( cond) ) );
	BOOST_CHECK_EQUAL( 0, value);

	boost::fibers::fiber s2(
            boost::bind(
                notify_one_fn,
                boost::ref( cond) ) );

	BOOST_CHECK_EQUAL( 0, value);

    if ( s1.joinable() ) s1.join();
    if ( s2.joinable() ) s2.join();
	BOOST_CHECK_EQUAL( 1, value);
}

void test_two_waiter_notify_one()
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

	value = 0;
	boost::fibers::mutex mtx;
	boost::fibers::condition cond;

    boost::fibers::fiber s1(
            boost::bind(
                wait_fn,
                boost::ref( mtx),
                boost::ref( cond) ) );
	BOOST_CHECK_EQUAL( 0, value);

    boost::fibers::fiber s2(
            boost::bind(
                wait_fn,
                boost::ref( mtx),
                boost::ref( cond) ) );
	BOOST_CHECK_EQUAL( 0, value);

    boost::fibers::fiber s3(
            boost::bind(
                notify_one_fn,
                boost::ref( cond) ) );
	BOOST_CHECK_EQUAL( 0, value);

    boost::fibers::fiber s4(
            boost::bind(
                notify_one_fn,
                boost::ref( cond) ) );
	BOOST_CHECK_EQUAL( 0, value);

    if ( s1.joinable() ) s1.join();
    if ( s2.joinable() ) s2.join();
    if ( s3.joinable() ) s3.join();
    if ( s4.joinable() ) s4.join();

	BOOST_CHECK_EQUAL( 2, value);
}

void test_two_waiter_notify_all()
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

	value = 0;
	boost::fibers::mutex mtx;
	boost::fibers::condition cond;

    boost::fibers::fiber s1(
            boost::bind(
                wait_fn,
                boost::ref( mtx),
                boost::ref( cond) ) );
	BOOST_CHECK_EQUAL( 0, value);

    boost::fibers::fiber s2(
            boost::bind(
                wait_fn,
                boost::ref( mtx),
                boost::ref( cond) ) );
	BOOST_CHECK_EQUAL( 0, value);

    boost::fibers::fiber s3(
            boost::bind(
                notify_all_fn,
                boost::ref( cond) ) );
	BOOST_CHECK_EQUAL( 0, value);

    boost::fibers::fiber s4(
            boost::bind(
                wait_fn,
                boost::ref( mtx),
                boost::ref( cond) ) );
	BOOST_CHECK_EQUAL( 0, value);

    boost::fibers::fiber s5(
            boost::bind(
                notify_all_fn,
                boost::ref( cond) ) );
	BOOST_CHECK_EQUAL( 0, value);

    if ( s1.joinable() ) s1.join();
    if ( s2.joinable() ) s2.join();
    if ( s3.joinable() ) s3.join();
    if ( s4.joinable() ) s4.join();
    if ( s5.joinable() ) s5.join();

	BOOST_CHECK_EQUAL( 3, value);
}

void test_condition_waits()
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    boost::fibers::fiber( do_test_condition_waits).join();
}

boost::unit_test::test_suite * init_unit_test_suite( int, char* [])
{
    boost::unit_test::test_suite * test =
        BOOST_TEST_SUITE("Boost.Fiber: condition test suite");

    test->add( BOOST_TEST_CASE( & test_one_waiter_notify_one) );
    test->add( BOOST_TEST_CASE( & test_two_waiter_notify_one) );
    test->add( BOOST_TEST_CASE( & test_two_waiter_notify_all) );
    test->add( BOOST_TEST_CASE( & test_condition_waits) );

	return test;
}
