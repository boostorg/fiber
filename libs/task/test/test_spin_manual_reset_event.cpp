
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
#include <boost/cstdint.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/function.hpp>
#include <boost/ref.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/thread.hpp>
#include <boost/utility.hpp>

#include <boost/task.hpp>

namespace pt = boost::posix_time;
namespace tsk = boost::tasks;

boost::uint32_t wait_fn( boost::uint32_t n, tsk::spin::manual_reset_event & ev)
{
	ev.wait();
	return n;
}

// check wait in new thread
void test_case_1()
{
	boost::uint32_t n = 3;
	tsk::spin::manual_reset_event ev;

	tsk::handle< boost::uint32_t > h1(
			tsk::async(
				tsk::make_task(
					wait_fn,
					n, boost::ref( ev) ),
				tsk::new_thread() ) );
	tsk::handle< boost::uint32_t > h2(
			tsk::async(
				tsk::make_task(
					wait_fn,
					n, boost::ref( ev) ),
				tsk::new_thread() ) );

	boost::this_thread::sleep( pt::millisec( 250) );
	BOOST_CHECK( ! h1.is_ready() );
	BOOST_CHECK( ! h2.is_ready() );

	ev.set();

	boost::this_thread::sleep( pt::millisec( 250) );
	BOOST_CHECK( h1.is_ready() );
	BOOST_CHECK( h2.is_ready() );
	BOOST_CHECK_EQUAL( h1.get(), n);
	BOOST_CHECK_EQUAL( h2.get(), n);

	ev.reset();

	tsk::handle< boost::uint32_t > h3(
			tsk::async(
				tsk::make_task(
					wait_fn,
					n, boost::ref( ev) ),
				tsk::new_thread() ) );
	tsk::handle< boost::uint32_t > h4(
			tsk::async(
				tsk::make_task(
					wait_fn,
					n, boost::ref( ev) ),
				tsk::new_thread() ) );

	boost::this_thread::sleep( pt::millisec( 250) );
	BOOST_CHECK( ! h3.is_ready() );
	BOOST_CHECK( ! h4.is_ready() );

	ev.set();

	boost::this_thread::sleep( pt::millisec( 250) );
	BOOST_CHECK( h3.is_ready() );
	BOOST_CHECK( h4.is_ready() );
	BOOST_CHECK_EQUAL( h3.get(), n);
	BOOST_CHECK_EQUAL( h4.get(), n);
}

// check wait in pool
void test_case_2()
{
	tsk::static_pool<
		tsk::unbounded_fifo
	> pool( tsk::poolsize( 3) );

	boost::uint32_t n = 3;
	tsk::spin::manual_reset_event ev;

	tsk::handle< boost::uint32_t > h1(
			tsk::async(
				tsk::make_task(
					wait_fn,
					n, boost::ref( ev) ),
				pool) );
	tsk::handle< boost::uint32_t > h2(
			tsk::async(
				tsk::make_task(
					wait_fn,
					n, boost::ref( ev) ),
				pool) );

	boost::this_thread::sleep( pt::millisec( 250) );
	BOOST_CHECK( ! h1.is_ready() );
	BOOST_CHECK( ! h2.is_ready() );

	ev.set();

	boost::this_thread::sleep( pt::millisec( 250) );
	BOOST_CHECK( h1.is_ready() );
	BOOST_CHECK( h2.is_ready() );
	BOOST_CHECK_EQUAL( h1.get(), n);
	BOOST_CHECK_EQUAL( h2.get(), n);

	ev.reset();

	tsk::handle< boost::uint32_t > h3(
			tsk::async(
				tsk::make_task(
					wait_fn,
					n, boost::ref( ev) ),
				tsk::new_thread() ) );
	tsk::handle< boost::uint32_t > h4(
			tsk::async(
				tsk::make_task(
					wait_fn,
					n, boost::ref( ev) ),
				tsk::new_thread() ) );

	boost::this_thread::sleep( pt::millisec( 250) );
	BOOST_CHECK( ! h3.is_ready() );
	BOOST_CHECK( ! h4.is_ready() );

	ev.set();

	boost::this_thread::sleep( pt::millisec( 250) );
	BOOST_CHECK( h3.is_ready() );
	BOOST_CHECK( h4.is_ready() );
	BOOST_CHECK_EQUAL( h3.get(), n);
	BOOST_CHECK_EQUAL( h4.get(), n);
}

void test_case_3()
{
	boost::uint32_t n = 3;
	tsk::spin::manual_reset_event ev( true);

	tsk::handle< boost::uint32_t > h1(
			tsk::async(
				tsk::make_task(
					wait_fn,
					n, boost::ref( ev) ),
				tsk::new_thread() ) );
	tsk::handle< boost::uint32_t > h2(
			tsk::async(
				tsk::make_task(
					wait_fn,
					n, boost::ref( ev) ),
				tsk::new_thread() ) );

	boost::this_thread::sleep( pt::millisec( 250) );
	BOOST_CHECK( h1.is_ready() );
	BOOST_CHECK( h2.is_ready() );
	BOOST_CHECK_EQUAL( h1.get(), n);
	BOOST_CHECK_EQUAL( h2.get(), n);

	ev.reset();

	tsk::handle< boost::uint32_t > h3(
			tsk::async(
				tsk::make_task(
					wait_fn,
					n, boost::ref( ev) ),
				tsk::new_thread() ) );
	tsk::handle< boost::uint32_t > h4(
			tsk::async(
				tsk::make_task(
					wait_fn,
					n, boost::ref( ev) ),
				tsk::new_thread() ) );

	boost::this_thread::sleep( pt::millisec( 250) );
	BOOST_CHECK( ! h3.is_ready() );
	BOOST_CHECK( ! h4.is_ready() );

	ev.set();

	boost::this_thread::sleep( pt::millisec( 250) );
	BOOST_CHECK( h3.is_ready() );
	BOOST_CHECK( h4.is_ready() );
	BOOST_CHECK_EQUAL( h3.get(), n);
	BOOST_CHECK_EQUAL( h4.get(), n);
}

void test_case_4()
{
	tsk::spin::manual_reset_event ev;

	BOOST_CHECK_EQUAL( false, ev.try_wait() );

	ev.set();

	BOOST_CHECK_EQUAL( true, ev.try_wait() );
	BOOST_CHECK_EQUAL( true, ev.try_wait() );
	ev.wait();
	BOOST_CHECK_EQUAL( true, ev.try_wait() );

	ev.reset();
	BOOST_CHECK_EQUAL( false, ev.try_wait() );
}

boost::unit_test::test_suite * init_unit_test_suite( int, char* [])
{
	boost::unit_test::test_suite * test =
		BOOST_TEST_SUITE("Boost.Task: spin-manual-reset-event test suite");

	test->add( BOOST_TEST_CASE( & test_case_1) );
	test->add( BOOST_TEST_CASE( & test_case_2) );
	test->add( BOOST_TEST_CASE( & test_case_3) );

	return test;
}
