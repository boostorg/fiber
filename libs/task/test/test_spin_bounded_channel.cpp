
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
#include <boost/optional.hpp>
#include <boost/ref.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/thread.hpp>
#include <boost/utility.hpp>

#include <boost/task.hpp>

namespace pt = boost::posix_time;
namespace tsk = boost::tasks;

struct send_data
{
	tsk::spin::bounded_channel< int >	&	buf;

	send_data( tsk::spin::bounded_channel< int > & buf_) :
		buf( buf_)
	{}

	void operator()( int value)
	{ buf.put( value); }
};

struct recv_data
{
	tsk::spin::bounded_channel< int >	&	buf;
	int									value;

	recv_data( tsk::spin::bounded_channel< int > & buf_) :
		buf( buf_), value( 0)
	{}

	void operator()( )
	{
		boost::optional< int > res;
		if ( buf.take( res) )
			value = * res;
	}
};

void test_case_1()
{
	tsk::spin::bounded_channel< int > buf( tsk::high_watermark( 10), tsk::low_watermark( 10) );
	BOOST_CHECK_EQUAL( true, buf.empty() );
	BOOST_CHECK_EQUAL( true, buf.active() );
	int n = 1;
	buf.put( n);
	BOOST_CHECK_EQUAL( false, buf.empty() );
	boost::optional< int > res;
	BOOST_CHECK_EQUAL( true, buf.take( res) );
	BOOST_CHECK( res);
	BOOST_CHECK_EQUAL( n, res.get() );
	buf.deactivate();
	BOOST_CHECK_EQUAL( false, buf.active() );
	BOOST_CHECK_THROW( buf.put( 1), std::runtime_error);
}

void test_case_2()
{
	tsk::spin::bounded_channel< int > buf( tsk::high_watermark( 10), tsk::low_watermark( 10) );
	BOOST_CHECK_EQUAL( true, buf.empty() );
	BOOST_CHECK_EQUAL( true, buf.active() );
	int n = 1;
	buf.put( n);
	BOOST_CHECK_EQUAL( false, buf.empty() );
	boost::optional< int > res;
	BOOST_CHECK_EQUAL( true, buf.try_take( res) );
	BOOST_CHECK( res);
	BOOST_CHECK_EQUAL( n, res.get() );
	BOOST_CHECK_EQUAL( false, buf.try_take( res) );
	BOOST_CHECK_EQUAL( false, buf.take( res, pt::milliseconds( 10) ) );
}

void test_case_3()
{
	tsk::static_pool<
		tsk::unbounded_fifo
	> pool( tsk::poolsize( 2) );

	tsk::spin::bounded_channel< int > buf( tsk::high_watermark( 10), tsk::low_watermark( 10) );

	int n = 37;

	recv_data receiver( buf);
	BOOST_CHECK_EQUAL( 0, receiver.value);
	tsk::handle< void > h =
		tsk::async(
			tsk::make_task(
				& recv_data::operator(),
				boost::ref( receiver) ),
			pool);

	boost::this_thread::sleep(
			pt::milliseconds( 250) );

	BOOST_CHECK_EQUAL( false, h.is_ready() );
	buf.put( n);

	h.wait();

	BOOST_CHECK_EQUAL( n, receiver.value);
}

void test_case_4()
{
	tsk::static_pool<
		tsk::unbounded_fifo
	> pool( tsk::poolsize( 2) );

	tsk::spin::bounded_channel< int > buf( tsk::high_watermark( 10), tsk::low_watermark( 10) );

	int n = 37;

	send_data sender( buf);
	recv_data receiver( buf);
	BOOST_CHECK_EQUAL( 0, receiver.value);

	tsk::handle< void > h1 =
		tsk::async(
			tsk::make_task(
				& recv_data::operator(),
				boost::ref( receiver) ),
			pool);

	boost::this_thread::sleep(
			pt::milliseconds( 250) );
	BOOST_CHECK_EQUAL( false, h1.is_ready() );

	tsk::handle< void > h2 =
		tsk::async(
			tsk::make_task(
				& send_data::operator(),
				boost::ref( sender),
				n),
			pool);

	h2.wait();
	BOOST_CHECK_EQUAL( true, h2.is_ready() );
	h1.wait();
	BOOST_CHECK_EQUAL( true, h1.is_ready() );

	BOOST_CHECK_EQUAL( n, receiver.value);
}

boost::unit_test::test_suite * init_unit_test_suite( int, char* [])
{
	boost::unit_test::test_suite * test =
		BOOST_TEST_SUITE("Boost.Task: bounded-buffer test suite");

	test->add( BOOST_TEST_CASE( & test_case_1) );
	test->add( BOOST_TEST_CASE( & test_case_2) );
	test->add( BOOST_TEST_CASE( & test_case_3) );
	test->add( BOOST_TEST_CASE( & test_case_4) );

	return test;
}
