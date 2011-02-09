
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
    typedef M mutex_type;
    typedef typename M::scoped_lock lock_type;

    void operator()( boost::tasklets::scheduler<> & sched)
    {
        mutex_type mtx;

        // Test the lock's constructors.
        {
            lock_type lk(mtx, boost::defer_lock);
            BOOST_CHECK(!lk);
        }
        lock_type lk(mtx);
        BOOST_CHECK(lk ? true : false);

        // Test the lock and unlock methods.
        lk.unlock();
        BOOST_CHECK(!lk);
        lk.lock();
        BOOST_CHECK(lk ? true : false);
    }
};

void do_test_mutex( boost::tasklets::scheduler<> & sched)
{
    test_lock< boost::tasklets::mutex >()( sched);
}

void test_case1()
{
	boost::tasklets::scheduler<> sched;
    sched.submit_tasklet(
		boost::tasklet(
			& do_test_mutex, boost::ref( sched), boost::tasklet::default_stacksize, boost::protected_stack_allocator()) );
	sched.run();
}

int value1 = 0;
int value2 = 0;

void test_fn1( boost::tasklets::mutex & mtx)
{
	boost::tasklets::mutex::scoped_lock lk( mtx);
	++value1;
	for ( int i = 0; i < 3; ++i)
		boost::this_tasklet::yield();
}

void test_fn2( boost::tasklets::mutex & mtx)
{
	++value2;
	boost::tasklets::mutex::scoped_lock lk( mtx);
	++value2;
}

void test_case2()
{
	boost::tasklets::scheduler<> sched;
	boost::tasklets::mutex mtx;
    sched.submit_tasklet(
		boost::tasklet(
			& test_fn1, boost::ref( mtx), boost::tasklet::default_stacksize, boost::protected_stack_allocator()) );
    sched.submit_tasklet(
		boost::tasklet(
			& test_fn2, boost::ref( mtx), boost::tasklet::default_stacksize, boost::protected_stack_allocator()) );

	BOOST_CHECK_EQUAL( 0, value1);
	BOOST_CHECK_EQUAL( 0, value2);

	BOOST_CHECK( sched.run() );
	BOOST_CHECK_EQUAL( std::size_t( 2), sched.size() );
	BOOST_CHECK_EQUAL( 1, value1);
	BOOST_CHECK_EQUAL( 0, value2);

	BOOST_CHECK( sched.run() );
	BOOST_CHECK_EQUAL( std::size_t( 2), sched.size() );
	BOOST_CHECK_EQUAL( 1, value1);
	BOOST_CHECK_EQUAL( 1, value2);

	BOOST_CHECK( sched.run() );
	BOOST_CHECK( sched.run() );
	BOOST_CHECK( sched.run() );
	BOOST_CHECK_EQUAL( std::size_t( 1), sched.size() );
	BOOST_CHECK_EQUAL( 1, value1);
	BOOST_CHECK_EQUAL( 1, value2);

	BOOST_CHECK( sched.run() );
	BOOST_CHECK_EQUAL( std::size_t( 0), sched.size() );
	BOOST_CHECK_EQUAL( 1, value1);
	BOOST_CHECK_EQUAL( 2, value2);

	BOOST_CHECK( ! sched.run() );
	BOOST_CHECK_EQUAL( std::size_t( 0), sched.size() );
	BOOST_CHECK_EQUAL( 1, value1);
	BOOST_CHECK_EQUAL( 2, value2);
}

boost::unit_test::test_suite * init_unit_test_suite( int, char* [])
{
    boost::unit_test_framework::test_suite * test =
		BOOST_TEST_SUITE("Boost.Tasklet: mutex test suite");

    test->add(BOOST_TEST_CASE(&test_case1));
    test->add(BOOST_TEST_CASE(&test_case2));

	return test;
}
