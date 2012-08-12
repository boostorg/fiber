
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

namespace stm = boost::fibers;
namespace this_stm = boost::this_fiber;

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

    stm::mutex mutex;
    stm::condition condition;
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

void notify_one_fn( stm::condition & cond)
{
	cond.notify_one();
}

void notify_all_fn( stm::condition & cond)
{
	cond.notify_all();
}

void wait_fn(
	stm::mutex & mtx,
	stm::condition & cond)
{
	stm::mutex::scoped_lock lk( mtx);
	cond.wait( lk);
	++value;
}

void condition_test_waits( condition_test_data * data)
{
    stm::mutex::scoped_lock lock( data->mutex);
    BOOST_CHECK( lock ? true : false);

    // Test wait.
    while ( data->notified != 1)
    {
        fprintf( stderr, "fiber will wait\n");
        data->condition.wait(lock);
        fprintf( stderr, "fiber notified\n");
    }
    fprintf( stderr, " data->notified == 1\n");
    BOOST_CHECK(lock ? true : false);
    BOOST_CHECK_EQUAL(data->notified, 1);
    data->awoken++;
    data->condition.notify_one();

    // Test predicate wait.
    data->condition.wait(lock, cond_predicate(data->notified, 2));
    BOOST_CHECK(lock ? true : false);
    BOOST_CHECK_EQUAL(data->notified, 2);
    data->awoken++;
    data->condition.notify_one();

    // Test timed_wait.
    boost::chrono::system_clock::time_point xt = delay(10);
    while (data->notified != 3)
        data->condition.timed_wait(lock, xt);
    BOOST_CHECK(lock ? true : false);
    BOOST_CHECK_EQUAL(data->notified, 3);
    data->awoken++;
    data->condition.notify_one();

    // Test predicate timed_wait.
    xt = delay(10);
    cond_predicate pred(data->notified, 4);
    BOOST_CHECK(data->condition.timed_wait(lock, xt, pred));
    BOOST_CHECK(lock ? true : false);
    BOOST_CHECK(pred());
    BOOST_CHECK_EQUAL(data->notified, 4);
    data->awoken++;
    data->condition.notify_one();

    // Test predicate timed_wait with relative timeout
    cond_predicate pred_rel(data->notified, 5);
    BOOST_CHECK(data->condition.timed_wait(lock, boost::chrono::seconds(10), pred_rel));
    BOOST_CHECK(lock ? true : false);
    BOOST_CHECK(pred_rel());
    BOOST_CHECK_EQUAL(data->notified, 5);
    data->awoken++;
    data->condition.notify_one();

    // Test timeout timed_wait.
    BOOST_CHECK(!data->condition.timed_wait(lock, boost::chrono::seconds(2)));
    BOOST_CHECK(lock ? true : false);
}

void do_test_condition_waits()
{
    condition_test_data data;

    stm::fiber s(
        stm::spawn(
            boost::bind( & condition_test_waits, & data) ) );

    {
        stm::mutex::scoped_lock lock( data.mutex);
        BOOST_CHECK(lock ? true : false);

        fprintf( stderr, "main increments data.notified\n");
        data.notified++;
        fprintf( stderr, "main notifies one fiber\n");
        data.condition.notify_one();
        while (data.awoken != 1)
        {
            fprintf( stderr, "main will wait\n");
            data.condition.wait(lock);
            fprintf( stderr, "main was notified\n");
        }
        fprintf( stderr, "main data.awoken == 1\n");
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

    s.join();
    BOOST_CHECK_EQUAL(data.awoken, 5);
}

void test_one_waiter_notify_one()
{
	value = 0;
	stm::mutex mtx;
	stm::condition cond;

    stm::fiber s1(
        stm::spawn(
            boost::bind(
                wait_fn,
                boost::ref( mtx),
                boost::ref( cond) ) ) );
	BOOST_CHECK_EQUAL( 0, value);

	BOOST_CHECK( ! stm::run() );
	BOOST_CHECK_EQUAL( 0, value);

	BOOST_CHECK( ! stm::run() );
	BOOST_CHECK_EQUAL( 0, value);

	stm::fiber s2(
        stm::spawn(
            boost::bind(
                notify_one_fn,
                boost::ref( cond) ) ) );
	BOOST_CHECK_EQUAL( 0, value);

	BOOST_CHECK( stm::run() );
	BOOST_CHECK_EQUAL( 1, value);

	BOOST_CHECK( ! stm::run() );
	BOOST_CHECK_EQUAL( 1, value);
}

void test_two_waiter_notify_one()
{
	value = 0;
	stm::mutex mtx;
	stm::condition cond;

    stm::fiber s1(
        stm::spawn(
            boost::bind(
                wait_fn,
                boost::ref( mtx),
                boost::ref( cond) ) ) );
	BOOST_CHECK_EQUAL( 0, value);

    stm::fiber s2(
        stm::spawn(
            boost::bind(
                wait_fn,
                boost::ref( mtx),
                boost::ref( cond) ) ) );
	BOOST_CHECK_EQUAL( 0, value);

	BOOST_CHECK( ! stm::run() );
	BOOST_CHECK_EQUAL( 0, value);

	BOOST_CHECK( ! stm::run() );
	BOOST_CHECK_EQUAL( 0, value);

	BOOST_CHECK( ! stm::run() );
	BOOST_CHECK_EQUAL( 0, value);

    stm::fiber s3(
        stm::spawn(
            boost::bind(
                notify_one_fn,
                boost::ref( cond) ) ) );
	BOOST_CHECK_EQUAL( 0, value);

	BOOST_CHECK( stm::run() );
	BOOST_CHECK_EQUAL( 1, value);

	BOOST_CHECK( ! stm::run() );
	BOOST_CHECK_EQUAL( 1, value);

    stm::fiber s4(
        stm::spawn(
            boost::bind(
                notify_one_fn,
                boost::ref( cond) ) ) );
	BOOST_CHECK_EQUAL( 1, value);

	BOOST_CHECK( stm::run() );
	BOOST_CHECK_EQUAL( 2, value);

	BOOST_CHECK( ! stm::run() );
	BOOST_CHECK_EQUAL( 2, value);
}

void test_two_waiter_notify_all()
{
	value = 0;
	stm::mutex mtx;
	stm::condition cond;

    stm::fiber s1(
        stm::spawn(
            boost::bind(
                wait_fn,
                boost::ref( mtx),
                boost::ref( cond) ) ) );
	BOOST_CHECK_EQUAL( 0, value);

    stm::fiber s2(
        stm::spawn(
            boost::bind(
                wait_fn,
                boost::ref( mtx),
                boost::ref( cond) ) ) );
	BOOST_CHECK_EQUAL( 0, value);

	BOOST_CHECK( ! stm::run() );
	BOOST_CHECK_EQUAL( 0, value);

	BOOST_CHECK( ! stm::run() );
	BOOST_CHECK_EQUAL( 0, value);

	BOOST_CHECK( ! stm::run() );
	BOOST_CHECK_EQUAL( 0, value);

    stm::fiber s3(
        stm::spawn(
            boost::bind(
                notify_all_fn,
                boost::ref( cond) ) ) );
	BOOST_CHECK_EQUAL( 0, value);

	BOOST_CHECK( stm::run() );
	BOOST_CHECK_EQUAL( 1, value);

	BOOST_CHECK( stm::run() );
	BOOST_CHECK_EQUAL( 2, value);

	BOOST_CHECK( ! stm::run() );
	BOOST_CHECK_EQUAL( 2, value);

    stm::fiber s4(
        stm::spawn(
            boost::bind(
                wait_fn,
                boost::ref( mtx),
                boost::ref( cond) ) ) );
	BOOST_CHECK_EQUAL( 2, value);

	BOOST_CHECK( ! stm::run() );
	BOOST_CHECK_EQUAL( 2, value);

	BOOST_CHECK( ! stm::run() );
	BOOST_CHECK_EQUAL( 2, value);

    stm::fiber s5(
        stm::spawn(
            boost::bind(
                notify_all_fn,
                boost::ref( cond) ) ) );
	BOOST_CHECK_EQUAL( 2, value);

	BOOST_CHECK( stm::run() );
	BOOST_CHECK_EQUAL( 3, value);

	BOOST_CHECK( ! stm::run() );
	BOOST_CHECK_EQUAL( 3, value);
}

void test_condition_waits()
{ do_test_condition_waits(); }

boost::unit_test::test_suite * init_unit_test_suite( int, char* [])
{
    boost::unit_test::test_suite * test =
        BOOST_TEST_SUITE("Boost.Stratified: condition test suite");
    test->add( BOOST_TEST_CASE( & test_one_waiter_notify_one) );
    test->add( BOOST_TEST_CASE( & test_two_waiter_notify_one) );
    test->add( BOOST_TEST_CASE( & test_two_waiter_notify_all) );
	test->add( BOOST_TEST_CASE( & test_condition_waits) );

	return test;
}
