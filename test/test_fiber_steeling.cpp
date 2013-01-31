
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cstdio>
#include <sstream>
#include <string>

#include <boost/atomic.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/thread.hpp>
#include <boost/utility.hpp>

#include <boost/fiber/all.hpp>

#define MAXCOUNT 50

#include <cstdio>
#include <sstream>

boost::atomic< bool > fini( false);
boost::fibers::round_robin * other_ds = 0;

boost::fibers::shared_future< int > fibonacci( int n);

int fibonacci_( int n)
{
    boost::this_fiber::yield();

    int res = 1;

    if ( 0 != n && 1 != n)
    {
        boost::fibers::shared_future< int > f1 = fibonacci( n - 1);
        boost::fibers::shared_future< int > f2 = fibonacci( n - 2);

        res = f1.get() + f2.get();
    }

    return res;
}

boost::fibers::shared_future< int > fibonacci( int n)
{
    boost::fibers::packaged_task<int> pt(
        boost::bind( fibonacci_, n) );
    boost::fibers::shared_future<int> f(pt.get_future());
    boost::fibers::fiber( boost::move(pt) ).detach();
    return f;
}

void create_fibers( int n)
{
    int res = fibonacci( n).get();
    fprintf( stderr, "fibonacci(%d) = %d\n", n, res);
}

void fn_create_fibers( boost::fibers::round_robin * ds, boost::barrier * b, int n)
{
    boost::fibers::scheduling_algorithm( ds);

    b->wait();

    boost::fibers::fiber f(
        boost::bind( create_fibers, n) );

    std::stringstream ss;
    ss << f.get_id();
    f.join();

    fini = true;
}

void fn_steel_fibers( boost::fibers::round_robin * other_ds, boost::barrier * b, int * count)
{
    BOOST_ASSERT( other_ds);
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

    b->wait();

    while ( ! fini)
    {
        boost::fibers::fiber f( other_ds->steel_from() );
        if ( f)
        {
            ++( * count);
            ds.migrate_to( f);
            while ( boost::fibers::detail::scheduler::instance().run() );
        }
        f.detach();
    }
}

void test_migrate_fiber()
{
    for ( int i = 0; i < MAXCOUNT; ++i) {
    fini = false;
	int n = 10, count = 0;

    boost::fibers::round_robin * ds = new boost::fibers::round_robin();
    boost::barrier b( 2);
    boost::thread t1( boost::bind( fn_create_fibers, ds, &b, n) );
    boost::thread t2( boost::bind( fn_steel_fibers, ds, &b, &count) );

    t1.join();
    t2.join();

    fprintf(stderr, "%d fibers stolen\n", count);
    fprintf(stderr, "%d. finished\n", i);
    delete ds;
    }
}

boost::unit_test::test_suite * init_unit_test_suite( int, char* [])
{
    boost::unit_test::test_suite * test =
        BOOST_TEST_SUITE("Boost.Fiber: round_robin test suite");

    test->add( BOOST_TEST_CASE( & test_migrate_fiber) );

    return test;
}
