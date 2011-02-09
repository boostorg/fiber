
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <vector>

#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/function.hpp>
#include <boost/ref.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/thread.hpp>
#include <boost/thread/barrier.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/utility.hpp>

#include <boost/task.hpp>

#include "test_functions.hpp"

namespace pt = boost::posix_time;
namespace tsk = boost::tasks;

// check size and move op
void test_case_1()
{
	tsk::static_pool<
		tsk::unbounded_fifo
	> pool1( tsk::poolsize( 3) );
	BOOST_CHECK( pool1);
	BOOST_CHECK_EQUAL( pool1.size(), std::size_t( 3) );

	tsk::static_pool<
		tsk::unbounded_fifo
	> pool2;
	BOOST_CHECK( ! pool2);
	BOOST_CHECK_THROW( pool2.size(), tsk::pool_moved);

	pool2 = boost::move( pool1);

	BOOST_CHECK( ! pool1);
	BOOST_CHECK_THROW( pool1.size(), tsk::pool_moved);

	BOOST_CHECK( pool2);
	BOOST_CHECK_EQUAL( pool2.size(), std::size_t( 3) );

	tsk::task< int > t( fibonacci_fn, 10);
	tsk::handle< int > h(
		tsk::async( boost::move( t), pool2) );
	BOOST_CHECK_EQUAL( h.get(), 55);
}

// check submit
void test_case_2()
{
	tsk::static_pool<
		tsk::unbounded_fifo
	> pool( tsk::poolsize( 3) );
	tsk::task< int > t( fibonacci_fn, 10);
	tsk::handle< int > h(
		tsk::async( boost::move( t), pool) );
	BOOST_CHECK_EQUAL( h.get(), 55);
}

// check assignment
void test_case_3()
{
	tsk::static_pool<
		tsk::unbounded_fifo
	> pool( tsk::poolsize( 3) );
	tsk::task< int > t( fibonacci_fn, 10);
	tsk::handle< int > h1;
	tsk::handle< int > h2(
		tsk::async( boost::move( t), pool) );
	h1 = h2;
	BOOST_CHECK_EQUAL( h1.get(), 55);
	BOOST_CHECK_EQUAL( h2.get(), 55);
}

// check swap
void test_case_4()
{
	tsk::static_pool<
		tsk::unbounded_fifo
	> pool( tsk::poolsize( 3) );
	tsk::task< int > t1( fibonacci_fn, 5);
	tsk::task< int > t2( fibonacci_fn, 10);
	tsk::handle< int > h1(
		tsk::async( boost::move( t1), pool) );
	tsk::handle< int > h2(
		tsk::async( boost::move( t2), pool) );
	BOOST_CHECK_EQUAL( h1.get(), 5);
	BOOST_CHECK_EQUAL( h2.get(), 55);
	BOOST_CHECK_NO_THROW( h1.swap( h2) );
	BOOST_CHECK_EQUAL( h1.get(), 55);
	BOOST_CHECK_EQUAL( h2.get(), 5);
}

// check runs in pool
void test_case_5()
{
	tsk::static_pool<
		tsk::unbounded_fifo
	> pool( tsk::poolsize( 1) );
	tsk::task< bool > t( runs_in_pool_fn);
	tsk::handle< bool > h(
		tsk::async( boost::move( t), pool) );
	BOOST_CHECK_EQUAL( h.get(), true);
}

// check shutdown
void test_case_6()
{
	tsk::static_pool<
		tsk::unbounded_fifo
	> pool( tsk::poolsize( 1) );
	tsk::task< int > t( fibonacci_fn, 10);
	tsk::handle< int > h(
		tsk::async( boost::move( t), pool) );
	pool.shutdown();
	BOOST_CHECK( pool.closed() );
	BOOST_CHECK_EQUAL( h.get(), 55);
}

// check runtime_error throw inside task
void test_case_7()
{
	tsk::static_pool<
		tsk::unbounded_fifo
	> pool( tsk::poolsize( 1) );
	tsk::task< void > t( throwing_fn);
	tsk::handle< void > h(
		tsk::async( boost::move( t), pool) );
	pool.shutdown();
	BOOST_CHECK_THROW( h.get(), std::runtime_error);
}

// check shutdown with task_rejected exception
void test_case_8()
{
	tsk::static_pool<
		tsk::unbounded_fifo
	> pool( tsk::poolsize( 1) );
	tsk::task< int > t( fibonacci_fn, 10);
	pool.shutdown();
	BOOST_CHECK( pool.closed() );
	BOOST_CHECK_THROW(
		tsk::async( boost::move( t), pool),
		tsk::task_rejected);
}

// check shutdown_now with thread_interrupted exception
void test_case_9()
{
	tsk::static_pool<
		tsk::unbounded_fifo
	> pool( tsk::poolsize( 1) );
	tsk::task< void > t( delay_fn, pt::millisec( 500) );
	tsk::handle< void > h(
		tsk::async( boost::move( t), pool) );
	boost::this_thread::sleep( pt::millisec( 250) );
	BOOST_CHECK_EQUAL( pool.size(), std::size_t( 1) );
	pool.shutdown_now();
	BOOST_CHECK( pool.closed() );
	BOOST_CHECK_EQUAL( pool.size(), std::size_t( 0) );
	BOOST_CHECK_THROW( h.get(), tsk::task_interrupted);
}

// check wait
void test_case_10()
{
	tsk::static_pool<
		tsk::unbounded_fifo
	> pool( tsk::poolsize( 3) );
	tsk::task< int > t( fibonacci_fn, 10);
	tsk::handle< int > h(
		tsk::async( boost::move( t), pool) );
	h.wait();
	BOOST_CHECK( h.is_ready() );
	BOOST_CHECK( h.has_value() );
	BOOST_CHECK( ! h.has_exception() );
	BOOST_CHECK_EQUAL( h.get(), 55);
}

// check wait_for
void test_case_11()
{
	tsk::static_pool<
		tsk::unbounded_fifo
	> pool( tsk::poolsize( 3) );
	tsk::task< void > t( delay_fn, pt::seconds( 1) );
	tsk::handle< void > h(
		tsk::async( boost::move( t), pool) );
	BOOST_CHECK( h.wait_for( pt::seconds( 3) ) );
	BOOST_CHECK( h.is_ready() );
	BOOST_CHECK( h.has_value() );
	BOOST_CHECK( ! h.has_exception() );
}

// check wait_for
void test_case_12()
{
	tsk::static_pool<
		tsk::unbounded_fifo
	> pool( tsk::poolsize( 3) );
	tsk::task< void > t( delay_fn, pt::seconds( 3) );
	tsk::handle< void > h(
		tsk::async( boost::move( t), pool) );
	BOOST_CHECK( ! h.wait_for( pt::seconds( 1) ) );
	BOOST_CHECK( ! h.is_ready() );
	BOOST_CHECK( ! h.has_value() );
	BOOST_CHECK( ! h.has_exception() );
}

// check wait_until
void test_case_13()
{
	tsk::static_pool<
		tsk::unbounded_fifo
	> pool( tsk::poolsize( 3) );
	tsk::task< void > t( delay_fn, pt::seconds( 1) );
	tsk::handle< void > h(
		tsk::async( boost::move( t), pool) );
	BOOST_CHECK( h.wait_until( boost::get_system_time() + pt::seconds( 3) ) );
	BOOST_CHECK( h.is_ready() );
	BOOST_CHECK( h.has_value() );
	BOOST_CHECK( ! h.has_exception() );
}

// check wait_until
void test_case_14()
{
	tsk::static_pool<
		tsk::unbounded_fifo
	> pool( tsk::poolsize( 3) );
	tsk::task< void > t( delay_fn, pt::seconds( 3) );
	tsk::handle< void > h(
		tsk::async( boost::move( t), pool) );
	BOOST_CHECK( ! h.wait_until( boost::get_system_time() + pt::seconds( 1) ) );
	BOOST_CHECK( ! h.is_ready() );
	BOOST_CHECK( ! h.has_value() );
	BOOST_CHECK( ! h.has_exception() );
}

// check interrupt
void test_case_15()
{
	tsk::static_pool<
		tsk::unbounded_fifo
	> pool( tsk::poolsize( 3) );
	tsk::task< void > t( delay_fn, pt::seconds( 3) );
	tsk::handle< void > h(
		tsk::async( boost::move( t), pool) );
	h.interrupt();
	BOOST_CHECK( h.interruption_requested() );
	BOOST_CHECK_THROW( h.get(), tsk::task_interrupted);
}

// check interrupt_all_worker
void test_case_16()
{
	tsk::static_pool<
		tsk::unbounded_fifo
	> pool( tsk::poolsize( 5) );
	tsk::task< void > t1( delay_fn, pt::seconds( 3) );
	tsk::task< void > t2( delay_fn, pt::seconds( 3) );
	tsk::task< void > t3( delay_fn, pt::seconds( 3) );
	tsk::handle< void > h1(
		tsk::async( boost::move( t1), pool) );
	tsk::handle< void > h2(
		tsk::async( boost::move( t2), pool) );
	tsk::handle< void > h3(
		tsk::async( boost::move( t3), pool) );
	boost::this_thread::sleep( pt::millisec( 250) );
	pool.interrupt_all_worker();
	BOOST_CHECK( ! h1.interruption_requested() );
	BOOST_CHECK( ! h2.interruption_requested() );
	BOOST_CHECK( ! h3.interruption_requested() );
	BOOST_CHECK_THROW( h1.get(), tsk::task_interrupted);
	BOOST_CHECK_THROW( h2.get(), tsk::task_interrupted);
	BOOST_CHECK_THROW( h3.get(), tsk::task_interrupted);
	BOOST_CHECK_EQUAL( pool.size(), std::size_t( 5) );
}

// check interrupt_and_wait
void test_case_17()
{
	tsk::static_pool<
		tsk::unbounded_fifo
	> pool( tsk::poolsize( 3) );
	bool finished( false);
	tsk::task< void > t(
		interrupt_fn,
		pt::seconds( 1),
		boost::ref( finished) );
	tsk::handle< void > h(
		tsk::async( boost::move( t), pool) );
	h.interrupt_and_wait();
	BOOST_CHECK( finished);
	BOOST_CHECK( h.is_ready() );
	BOOST_CHECK( ! h.has_value() );
	BOOST_CHECK( h.has_exception() );
	BOOST_CHECK( h.interruption_requested() );
	BOOST_CHECK_THROW( h.get(), tsk::task_interrupted);
}

// check interrupt_and_wait_for
void test_case_18()
{
	tsk::static_pool<
		tsk::unbounded_fifo
	> pool( tsk::poolsize( 3) );
	bool finished( false);
	tsk::task< void > t(
		interrupt_fn,
		pt::seconds( 1),
		boost::ref( finished) );
	tsk::handle< void > h(
		tsk::async( boost::move( t), pool) );
	BOOST_CHECK( h.interrupt_and_wait_for( pt::seconds( 3) ) );
	BOOST_CHECK( finished);
	BOOST_CHECK( h.is_ready() );
	BOOST_CHECK( ! h.has_value() );
	BOOST_CHECK( h.has_exception() );
	BOOST_CHECK( h.interruption_requested() );
	BOOST_CHECK_THROW( h.get(), tsk::task_interrupted);
}

// check interrupt_and_wait_for
void test_case_19()
{
	tsk::static_pool<
		tsk::unbounded_fifo
	> pool( tsk::poolsize( 3) );
	tsk::task< void > t( non_interrupt_fn, 3);
	tsk::handle< void > h(
		tsk::async( boost::move( t), pool) );
	BOOST_CHECK( ! h.interrupt_and_wait_for( pt::seconds( 1) ) );
}

// check interrupt_and_wait_until
void test_case_20()
{
	tsk::static_pool<
		tsk::unbounded_fifo
	> pool( tsk::poolsize( 3) );
	bool finished( false);
	tsk::task< void > t(
		interrupt_fn,
		pt::seconds( 1),
		boost::ref( finished) );
	tsk::handle< void > h(
		tsk::async( boost::move( t), pool) );
	BOOST_CHECK( h.interrupt_and_wait_until( boost::get_system_time() + pt::seconds( 3) ) );
	BOOST_CHECK( finished);
	BOOST_CHECK( h.is_ready() );
	BOOST_CHECK( ! h.has_value() );
	BOOST_CHECK( h.has_exception() );
	BOOST_CHECK( h.interruption_requested() );
	BOOST_CHECK_THROW( h.get(), tsk::task_interrupted);
}

// check interrupt_and_wait_until
void test_case_21()
{
	tsk::static_pool<
		tsk::unbounded_fifo
	> pool( tsk::poolsize( 3) );
	tsk::task< void > t( non_interrupt_fn, 3);
	tsk::handle< void > h(
		tsk::async( boost::move( t), pool) );
	BOOST_CHECK( ! h.interrupt_and_wait_until( boost::get_system_time() + pt::seconds( 1) ) );
}

// check fifo scheduling
void test_case_22()
{
	typedef tsk::static_pool<
		tsk::unbounded_fifo
	> pool_type;
	BOOST_CHECK( ! tsk::has_attribute< pool_type >::value);
	pool_type pool( tsk::poolsize( 1) );
	boost::barrier b( 2);
	std::vector< int > buffer;
	tsk::task< void > t1( barrier_fn, boost::ref( b) );
	tsk::task< void > t2(
		buffer_fibonacci_fn,
		boost::ref( buffer),
		10);
	tsk::task< void > t3(
		buffer_fibonacci_fn,
		boost::ref( buffer),
		0);
	tsk::async( boost::move( t1), pool);
	boost::this_thread::sleep( pt::millisec( 250) );
	tsk::async( boost::move( t2), pool);
	tsk::async( boost::move( t3), pool);
	b.wait();
	pool.shutdown();
	BOOST_CHECK_EQUAL( buffer[0], 55);
	BOOST_CHECK_EQUAL( buffer[1], 0);
	BOOST_CHECK_EQUAL( buffer.size(), std::size_t( 2) );
}

boost::unit_test::test_suite * init_unit_test_suite( int, char* [])
{
	boost::unit_test::test_suite * test =
		BOOST_TEST_SUITE("Boost.Task: unbounded-pool test suite");

	test->add( BOOST_TEST_CASE( & test_case_1) );
	test->add( BOOST_TEST_CASE( & test_case_2) );
	test->add( BOOST_TEST_CASE( & test_case_3) );
	test->add( BOOST_TEST_CASE( & test_case_4) );
	test->add( BOOST_TEST_CASE( & test_case_5) );
	test->add( BOOST_TEST_CASE( & test_case_6) );
	test->add( BOOST_TEST_CASE( & test_case_7) );
	test->add( BOOST_TEST_CASE( & test_case_8) );
	test->add( BOOST_TEST_CASE( & test_case_9) );
	test->add( BOOST_TEST_CASE( & test_case_10) );
	test->add( BOOST_TEST_CASE( & test_case_11) );
	test->add( BOOST_TEST_CASE( & test_case_12) );
	test->add( BOOST_TEST_CASE( & test_case_13) );
	test->add( BOOST_TEST_CASE( & test_case_14) );
	test->add( BOOST_TEST_CASE( & test_case_15) );
	test->add( BOOST_TEST_CASE( & test_case_16) );
	test->add( BOOST_TEST_CASE( & test_case_17) );
	test->add( BOOST_TEST_CASE( & test_case_18) );
	test->add( BOOST_TEST_CASE( & test_case_19) );
	test->add( BOOST_TEST_CASE( & test_case_20) );
	test->add( BOOST_TEST_CASE( & test_case_21) );
	test->add( BOOST_TEST_CASE( & test_case_22) );

	return test;
}
