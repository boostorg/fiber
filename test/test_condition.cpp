
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// This test is based on the tests of Boost.Thread 

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

typedef boost::chrono::nanoseconds  ns;
typedef boost::chrono::milliseconds ms;

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

void condition_test_fiber(condition_test_data* data)
{
    boost::unique_lock<boost::fibers::mutex> lock(data->mutex);
    BOOST_CHECK(lock ? true : false);
    while (!(data->notified > 0))
        data->condition.wait(lock);
    BOOST_CHECK(lock ? true : false);
    data->awoken++;
}

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
	boost::unique_lock< boost::fibers::mutex > lk( mtx);
	cond.wait( lk);
	++value;
}

void test_condition_wait_is_a_interruption_point()
{
    condition_test_data data;
    bool interrupted = false;
    boost::fibers::fiber f(boost::bind(&condition_test_fiber, &data));

    f.interrupt();
    try
    { f.join(); }
    catch ( boost::fibers::fiber_interrupted const&)
    { interrupted = true; }
    BOOST_CHECK(interrupted);
    BOOST_CHECK_EQUAL(data.awoken,0);
}

void test_one_waiter_notify_one()
{
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

    s1.join();
    s2.join();

	BOOST_CHECK_EQUAL( 1, value);
}

void test_two_waiter_notify_one()
{
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

    s1.join();
    s2.join();
    s3.join();
    s4.join();

	BOOST_CHECK_EQUAL( 2, value);
}

void test_two_waiter_notify_all()
{
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

    s1.join();
    s2.join();
    s3.join();
    s4.join();
    s5.join();

	BOOST_CHECK_EQUAL( 3, value);
}

int test1 = 0;
int test2 = 0;

int runs = 0;

void fn1( boost::fibers::mutex & m, boost::fibers::condition_variable & cv)
{
    boost::unique_lock< boost::fibers::mutex > lk( m);
    BOOST_CHECK(test2 == 0);
    test1 = 1;
    cv.notify_one();
    while (test2 == 0) {
        cv.wait(lk);
    }
    BOOST_CHECK(test2 != 0);
}

void fn2( boost::fibers::mutex & m, boost::fibers::condition_variable & cv)
{
    boost::unique_lock< boost::fibers::mutex > lk( m);
    BOOST_CHECK(test2 == 0);
    test1 = 1;
    cv.notify_one();
    boost::fibers::clock_type::time_point t0 = boost::fibers::clock_type::now();
    boost::fibers::clock_type::time_point t = t0 + ms(250);
    int count=0;
    while (test2 == 0 && cv.wait_until(lk, t) == boost::fibers::cv_status::no_timeout)
        count++;
    boost::fibers::clock_type::time_point t1 = boost::fibers::clock_type::now();
    if (runs == 0)
    {
        BOOST_CHECK(t1 - t0 < ms(250));
        BOOST_CHECK(test2 != 0);
    }
    else
    {
        BOOST_CHECK(t1 - t0 - ms(250) < ms(count*250+5+1000));
        BOOST_CHECK(test2 == 0);
    }
    ++runs;
}

class Pred
{
     int    &   i_;

public:
    explicit Pred(int& i) :
        i_(i)
    {}

    bool operator()()
    { return i_ != 0; }
};

void fn3( boost::fibers::mutex & m, boost::fibers::condition_variable & cv)
{
    boost::unique_lock< boost::fibers::mutex > lk( m);
    BOOST_CHECK(test2 == 0);
    test1 = 1;
    cv.notify_one();
    boost::fibers::clock_type::time_point t0 = boost::fibers::clock_type::now();
    boost::fibers::clock_type::time_point t = t0 + ms(250);
    bool r = cv.wait_until(lk, t, Pred(test2));
    boost::fibers::clock_type::time_point t1 = boost::fibers::clock_type::now();
    if (runs == 0)
    {
        BOOST_CHECK(t1 - t0 < ms(250));
        BOOST_CHECK(test2 != 0);
        BOOST_CHECK(r);
    }
    else
    {
        BOOST_CHECK(t1 - t0 - ms(250) < ms(250+2));
        BOOST_CHECK(test2 == 0);
        BOOST_CHECK(!r);
    }
    ++runs;
}

void fn4( boost::fibers::mutex & m, boost::fibers::condition_variable & cv)
{
    boost::unique_lock< boost::fibers::mutex > lk( m);
    BOOST_CHECK(test2 == 0);
    test1 = 1;
    cv.notify_one();
    boost::fibers::clock_type::time_point t0 = boost::fibers::clock_type::now();
    int count=0;
    while (test2 == 0 && cv.wait_for(lk, ms(250)) == boost::fibers::cv_status::no_timeout)
        count++;
    boost::fibers::clock_type::time_point t1 = boost::fibers::clock_type::now();
    if (runs == 0)
    {
        BOOST_CHECK(t1 - t0 < ms(250));
        BOOST_CHECK(test2 != 0);
    }
    else
    {
        BOOST_CHECK(t1 - t0 - ms(250) < ms(count*250+5+1000));
        BOOST_CHECK(test2 == 0);
    }
    ++runs;
}

void fn5( boost::fibers::mutex & m, boost::fibers::condition_variable & cv)
{
    boost::unique_lock< boost::fibers::mutex > lk( m);
    BOOST_CHECK(test2 == 0);
    test1 = 1;
    cv.notify_one();
    boost::fibers::clock_type::time_point t0 = boost::fibers::clock_type::now();
    int count=0;
    cv.wait_for(lk, ms(250), Pred(test2));
    count++;
    boost::fibers::clock_type::time_point t1 = boost::fibers::clock_type::now();
    if (runs == 0)
    {
        BOOST_CHECK(t1 - t0 < ms(250+1000));
        BOOST_CHECK(test2 != 0);
    }
    else
    {
        BOOST_CHECK(t1 - t0 - ms(250) < ms(count*250+2));
        BOOST_CHECK(test2 == 0);
    }
    ++runs;
}

void do_test_condition_wait()
{
    test1 = 0;
    test2 = 0;
    runs = 0;

    boost::fibers::mutex m;
    boost::fibers::condition_variable cv;
    boost::unique_lock< boost::fibers::mutex > lk( m);
    boost::fibers::fiber f( boost::bind( & fn1, boost::ref( m), boost::ref( cv) ) );
    BOOST_CHECK(test1 == 0);
    while (test1 == 0)
        cv.wait(lk);
    BOOST_CHECK(test1 != 0);
    test2 = 1;
    lk.unlock();
    cv.notify_one();
    f.join();
}

void test_condition_wait()
{
    boost::fibers::fiber( & do_test_condition_wait).join();
    do_test_condition_wait();
}

void do_test_condition_wait_until()
{
    test1 = 0;
    test2 = 0;
    runs = 0;

    boost::fibers::mutex m;
    boost::fibers::condition_variable cv;
    {
        boost::unique_lock< boost::fibers::mutex > lk( m);
        boost::fibers::fiber f( boost::bind( & fn2, boost::ref( m), boost::ref( cv) ) );
        BOOST_CHECK(test1 == 0);
        while (test1 == 0)
            cv.wait(lk);
        BOOST_CHECK(test1 != 0);
        test2 = 1;
        lk.unlock();
        cv.notify_one();
        f.join();
    }
    test1 = 0;
    test2 = 0;
    {
        boost::unique_lock< boost::fibers::mutex > lk( m);
        boost::fibers::fiber f( boost::bind( & fn2, boost::ref( m), boost::ref( cv) ) );
        BOOST_CHECK(test1 == 0);
        while (test1 == 0)
            cv.wait(lk);
        BOOST_CHECK(test1 != 0);
        lk.unlock();
        f.join();
    }
}

void test_condition_wait_until()
{
    boost::fibers::fiber( & do_test_condition_wait_until).join();
    do_test_condition_wait_until();
}

void do_test_condition_wait_until_pred()
{
    test1 = 0;
    test2 = 0;
    runs = 0;

    boost::fibers::mutex m;
    boost::fibers::condition_variable cv;
    {
        boost::unique_lock< boost::fibers::mutex > lk( m);
        boost::fibers::fiber f( boost::bind( & fn3, boost::ref( m), boost::ref( cv) ) );
        BOOST_CHECK(test1 == 0);
        while (test1 == 0)
            cv.wait(lk);
        BOOST_CHECK(test1 != 0);
        test2 = 1;
        lk.unlock();
        cv.notify_one();
        f.join();
    }
    test1 = 0;
    test2 = 0;
    {
        boost::unique_lock< boost::fibers::mutex > lk( m);
        boost::fibers::fiber f( boost::bind( & fn3, boost::ref( m), boost::ref( cv) ) );
        BOOST_CHECK(test1 == 0);
        while (test1 == 0)
            cv.wait(lk);
        BOOST_CHECK(test1 != 0);
        lk.unlock();
        f.join();
    }
}

void test_condition_wait_until_pred()
{
    boost::fibers::fiber( & do_test_condition_wait_until_pred).join();
    do_test_condition_wait_until_pred();
}

void do_test_condition_wait_for()
{
    test1 = 0;
    test2 = 0;
    runs = 0;

    boost::fibers::mutex m;
    boost::fibers::condition_variable cv;
    {
        boost::unique_lock< boost::fibers::mutex > lk( m);
        boost::fibers::fiber f( boost::bind( & fn4, boost::ref( m), boost::ref( cv) ) );
        BOOST_CHECK(test1 == 0);
        while (test1 == 0)
            cv.wait(lk);
        BOOST_CHECK(test1 != 0);
        test2 = 1;
        lk.unlock();
        cv.notify_one();
        f.join();
    }
    test1 = 0;
    test2 = 0;
    {
        boost::unique_lock< boost::fibers::mutex > lk( m);
        boost::fibers::fiber f( boost::bind( & fn4, boost::ref( m), boost::ref( cv) ) );
        BOOST_CHECK(test1 == 0);
        while (test1 == 0)
            cv.wait(lk);
        BOOST_CHECK(test1 != 0);
        lk.unlock();
        f.join();
    }
}

void test_condition_wait_for()
{
    boost::fibers::fiber( & do_test_condition_wait_for).join();
    do_test_condition_wait_for();
}

void do_test_condition_wait_for_pred()
{
    test1 = 0;
    test2 = 0;
    runs = 0;

    boost::fibers::mutex m;
    boost::fibers::condition_variable cv;
    {
        boost::unique_lock< boost::fibers::mutex > lk( m);
        boost::fibers::fiber f( boost::bind( & fn5, boost::ref( m), boost::ref( cv) ) );
        BOOST_CHECK(test1 == 0);
        while (test1 == 0)
            cv.wait(lk);
        BOOST_CHECK(test1 != 0);
        test2 = 1;
        lk.unlock();
        cv.notify_one();
        f.join();
    }
    test1 = 0;
    test2 = 0;
    {
        boost::unique_lock< boost::fibers::mutex > lk( m);
        boost::fibers::fiber f( boost::bind( & fn5, boost::ref( m), boost::ref( cv) ) );
        BOOST_CHECK(test1 == 0);
        while (test1 == 0)
            cv.wait(lk);
        BOOST_CHECK(test1 != 0);
        lk.unlock();
        f.join();
    }
}

void test_condition_wait_for_pred()
{
    boost::fibers::fiber( & do_test_condition_wait_for_pred).join();
    do_test_condition_wait_for_pred();
}

boost::unit_test::test_suite * init_unit_test_suite( int, char* [])
{
    boost::unit_test::test_suite * test =
        BOOST_TEST_SUITE("Boost.Fiber: condition test suite");

    test->add( BOOST_TEST_CASE( & test_one_waiter_notify_one) );
    test->add( BOOST_TEST_CASE( & test_two_waiter_notify_one) );
    test->add( BOOST_TEST_CASE( & test_two_waiter_notify_all) );
    test->add( BOOST_TEST_CASE( & test_condition_wait) );
    test->add( BOOST_TEST_CASE( & test_condition_wait_is_a_interruption_point) );
    test->add( BOOST_TEST_CASE( & test_condition_wait_until) );
    test->add( BOOST_TEST_CASE( & test_condition_wait_until_pred) );
    test->add( BOOST_TEST_CASE( & test_condition_wait_for) );
    test->add( BOOST_TEST_CASE( & test_condition_wait_for_pred) );

	return test;
}
