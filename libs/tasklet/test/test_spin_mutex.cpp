
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// This test is based on the tests of Boost.Thread 

#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <vector>

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/ref.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/utility.hpp>

#include <boost/tasklet.hpp>

template< typename M >
struct test_lock
{
    typedef M spin_mutex_type;
    typedef typename M::scoped_lock lock_type;

    void operator()()
    {
        spin_mutex_type spin_mutex;
        boost::tasklets::spin_condition condition;

        // Test the lock's constructors.
        {
            lock_type lock(spin_mutex, boost::defer_lock);
            BOOST_CHECK(!lock);
        }
        lock_type lock(spin_mutex);
        BOOST_CHECK(lock ? true : false);

        // Test the lock and unlock methods.
        lock.unlock();
        BOOST_CHECK(!lock);
        lock.lock();
        BOOST_CHECK(lock ? true : false);
    }
};

void do_test_spin_mutex()
{
    test_lock< boost::tasklets::spin_mutex >()();
}

void test_case1()
{
	boost::tasklets::scheduler<> sched;
    sched.submit_tasklet( boost::tasklet( & do_test_spin_mutex, boost::tasklet::default_stacksize, boost::protected_stack_allocator()) );
	sched.run();
}

int value1 = 0;
int value2 = 0;

void test_fn1( boost::tasklets::spin_mutex & mtx)
{
	boost::tasklets::spin_mutex::scoped_lock lk( mtx);
	++value1;
	for ( int i = 0; i < 3; ++i)
		boost::this_tasklet::yield();
}

void test_fn2( boost::tasklets::spin_mutex & mtx)
{
	boost::tasklets::spin_mutex::scoped_lock lk( mtx);
	++value2;
}

void test_case2()
{
	boost::tasklets::spin_mutex mtx;
	boost::tasklets::scheduler<> sched;
    sched.submit_tasklet( boost::tasklet( & test_fn1, boost::ref( mtx), boost::tasklet::default_stacksize, boost::protected_stack_allocator()));
    sched.submit_tasklet( boost::tasklet( & test_fn2, boost::ref( mtx), boost::tasklet::default_stacksize, boost::protected_stack_allocator()));

	BOOST_CHECK_EQUAL( 0, value1);
	BOOST_CHECK_EQUAL( 0, value2);

	BOOST_CHECK( sched.run() );
	BOOST_CHECK_EQUAL( std::size_t( 2), sched.size() );
	BOOST_CHECK_EQUAL( 1, value1);
	BOOST_CHECK_EQUAL( 0, value2);

	BOOST_CHECK( sched.run() );
	BOOST_CHECK_EQUAL( std::size_t( 2), sched.size() );
	BOOST_CHECK_EQUAL( 1, value1);
	BOOST_CHECK_EQUAL( 0, value2);

	BOOST_CHECK( sched.run() );
	BOOST_CHECK( sched.run() );
	BOOST_CHECK( sched.run() );
	BOOST_CHECK_EQUAL( std::size_t( 2), sched.size() );
	BOOST_CHECK_EQUAL( 1, value1);
	BOOST_CHECK_EQUAL( 0, value2);

	BOOST_CHECK( sched.run() );
	BOOST_CHECK_EQUAL( std::size_t( 2), sched.size() );
	BOOST_CHECK_EQUAL( 1, value1);
	BOOST_CHECK_EQUAL( 0, value2);

	BOOST_CHECK( sched.run() );
	BOOST_CHECK_EQUAL( std::size_t( 1), sched.size() );
	BOOST_CHECK_EQUAL( 1, value1);
	BOOST_CHECK_EQUAL( 0, value2);

	BOOST_CHECK( sched.run() );
	BOOST_CHECK_EQUAL( std::size_t( 0), sched.size() );
	BOOST_CHECK_EQUAL( 1, value1);
	BOOST_CHECK_EQUAL( 1, value2);
}

boost::unit_test::test_suite * init_unit_test_suite( int, char* [])
{
    boost::unit_test_framework::test_suite * test =
		BOOST_TEST_SUITE("Boost.Tasklet: spin-mutex test suite");

    test->add(BOOST_TEST_CASE(&test_case1));
    test->add(BOOST_TEST_CASE(&test_case2));

	return test;
}
