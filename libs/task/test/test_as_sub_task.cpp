
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
#include <boost/utility.hpp>

#include <boost/task.hpp>

#include "test_functions.hpp"

namespace pt = boost::posix_time;
namespace tsk = boost::tasks;

namespace {

bool exec_sub_task()
{
	tsk::handle< bool > h(
		tsk::async(
			tsk::make_task( runs_in_pool_fn),
			tsk::as_sub_task() ) );
	return h.get();	
}

void test_runs_not_in_pool()
{
	tsk::task< bool > t( runs_in_pool_fn);
	tsk::handle< bool > h(
		tsk::async( boost::move( t), tsk::as_sub_task() ) );
	BOOST_CHECK_EQUAL( h.get(), false);
}

void test_runs_in_pool()
{
	tsk::static_pool<
		tsk::unbounded_fifo
	> pool( tsk::poolsize( 1) );
	tsk::handle< bool > h(
		tsk::async(
			tsk::make_task( exec_sub_task),
			pool) );
	BOOST_CHECK_EQUAL( h.get(), true);
}

}

boost::unit_test::test_suite * init_unit_test_suite( int, char* [])
{
    boost::unit_test_framework::test_suite * test =
		BOOST_TEST_SUITE("Boost.Task: as-sub-task test suite");

	test->add( BOOST_TEST_CASE( & test_runs_in_pool) );
	test->add( BOOST_TEST_CASE( & test_runs_not_in_pool) );

	return test;
}
