
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

boost::uint32_t wait_fn( boost::uint32_t n, tsk::spin::count_down_event & ev)
{
	ev.wait();
	return n;
}

// check initial + current
void test_case_1()
{
	boost::uint32_t n = 3;
	tsk::spin::count_down_event ev( n);
	BOOST_CHECK_EQUAL( ev.initial(), n);
	BOOST_CHECK_EQUAL( ev.current(), n);

	ev.set();
	BOOST_CHECK_EQUAL( ev.initial(), n);
	BOOST_CHECK_EQUAL( ev.current(), static_cast< boost::uint32_t >( 2) );

	ev.set();
	BOOST_CHECK_EQUAL( ev.initial(), n);
	BOOST_CHECK_EQUAL( ev.current(), static_cast< boost::uint32_t >( 1) );

	ev.set();
	BOOST_CHECK_EQUAL( ev.initial(), n);
	BOOST_CHECK_EQUAL( ev.current(), static_cast< boost::uint32_t >( 0) );

	ev.set();
	BOOST_CHECK_EQUAL( ev.initial(), n);
	BOOST_CHECK_EQUAL( ev.current(), static_cast< boost::uint32_t >( 0) );
}

// check wait in new thread
void test_case_2()
{
	boost::uint32_t n = 3;
	tsk::spin::count_down_event ev( n);
	BOOST_CHECK_EQUAL( ev.initial(), n);
	BOOST_CHECK_EQUAL( ev.current(), n);

	tsk::handle< boost::uint32_t > h(
			tsk::async(
				tsk::make_task(
					wait_fn,
					n, boost::ref( ev) ),
				tsk::new_thread() ) );
	BOOST_CHECK( ! h.is_ready() );
	for ( boost::uint32_t i = 0; i < n; ++i)
	{
		ev.set();
		BOOST_CHECK( ! h.is_ready() );
	}
	BOOST_CHECK_EQUAL( ev.initial(), n);
	BOOST_CHECK_EQUAL( ev.current(), static_cast< boost::uint32_t >( 0) );
	BOOST_CHECK_EQUAL( h.get(), n);
}

// check wait in pool
void test_case_3()
{
	tsk::static_pool<
		tsk::unbounded_fifo
	> pool( tsk::poolsize( 3) );

	boost::uint32_t n = 3;
	tsk::spin::count_down_event ev( n);
	BOOST_CHECK_EQUAL( ev.initial(), n);
	BOOST_CHECK_EQUAL( ev.current(), n);

	tsk::handle< boost::uint32_t > h(
			tsk::async(
				tsk::make_task(
					wait_fn,
					n, boost::ref( ev) ),
				pool) );
	BOOST_CHECK( ! h.is_ready() );
	for ( boost::uint32_t i = 0; i < n; ++i)
	{
		ev.set();
		BOOST_CHECK( ! h.is_ready() );
	}
	BOOST_CHECK_EQUAL( ev.initial(), n);
	BOOST_CHECK_EQUAL( ev.current(), static_cast< boost::uint32_t >( 0) );
	BOOST_CHECK_EQUAL( h.get(), n);
}

boost::unit_test::test_suite * init_unit_test_suite( int, char* [])
{
	boost::unit_test::test_suite * test =
		BOOST_TEST_SUITE("Boost.Task: spin-count-down-event test suite");

	test->add( BOOST_TEST_CASE( & test_case_1) );
	test->add( BOOST_TEST_CASE( & test_case_2) );
	test->add( BOOST_TEST_CASE( & test_case_3) );

	return test;
}
