
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cstdio>
#include <sstream>
#include <string>

#include <boost/atomic.hpp>
#include <boost/assert.hpp>
#include <boost/thread.hpp>
#include <boost/utility.hpp>

#include <boost/fiber/all.hpp>
#include "workstealing_round_robin.hpp"

#define MAXCOUNT 100

boost::atomic< bool > fini( false);
workstealing_round_robin * other_ds = 0;

boost::fibers::future< int > fibonacci( int);

int fibonacci_( int n)
{
    boost::this_fiber::yield();

    int res = 1;

    if ( 0 != n && 1 != n)
    {
        boost::fibers::future< int > f1 = fibonacci( n - 1);
        boost::fibers::future< int > f2 = fibonacci( n - 2);

        res = f1.get() + f2.get();
    }

    return res;
}

boost::fibers::future< int > fibonacci( int n)
{
    boost::fibers::packaged_task< int() > pt( boost::bind( fibonacci_, n) );
    boost::fibers::future< int > f( pt.get_future() );
    boost::fibers::fiber( boost::move( pt) ).detach();
    return boost::move( f);
}

int create_fiber( int n)
{
    return fibonacci( n).get();
}

void fn_create_fibers( workstealing_round_robin * ds, boost::barrier * b)
{
    boost::fibers::set_scheduling_algorithm( ds);

    b->wait();

    int result = boost::fibers::async( boost::bind( create_fiber, 10) ).get();
    BOOST_ASSERT( 89 == result);
    fprintf( stderr, "fibonacci(10) = %d", result);

    fini = true;
}

void fn_migrate_fibers( workstealing_round_robin * other_ds, boost::barrier * b, int * count)
{
    BOOST_ASSERT( other_ds);

    workstealing_round_robin ds;
    boost::fibers::set_scheduling_algorithm( & ds);

    b->wait();

    while ( ! fini)
    {
        // To guarantee progress, we must ensure that
        // threads that have work to do are not unreasonably delayed by (thief) threads
        // which are idle except for task-stealing. 
        // This call yields the thief ’s processor to another thread, allowing
        // descheduled threads to regain a processor and make progress. 
        boost::this_thread::yield();

        boost::fibers::fiber f( other_ds->steal_from() );
        if ( f)
        {
            ++( * count);
            ds.migrate_to( f);
            f.detach();
        }
        while ( boost::fibers::detail::scheduler::instance()->run() );
    }
}

void test_migrate_fiber()
{
    for ( int i = 0; i < MAXCOUNT; ++i) {
        fprintf(stderr, "%d. ", i);

        fini = false;
        int count = 0;

        workstealing_round_robin * ds = new workstealing_round_robin();
        boost::barrier b( 2);
        boost::thread t1( boost::bind( fn_create_fibers, ds, &b) );
        boost::thread t2( boost::bind( fn_migrate_fibers, ds, &b, &count) );

        t1.join();
        t2.join();

        fprintf(stderr, ", %d fibers stolen\n", count);
        delete ds;
    }
}

int main()
{
    try
    {
        for ( int i = 0; i < MAXCOUNT; ++i) {
            fprintf(stderr, "%d. ", i);

            fini = false;
            int count = 0;

            workstealing_round_robin * ds = new workstealing_round_robin();
            boost::barrier b( 2);
            boost::thread t1( boost::bind( fn_create_fibers, ds, &b) );
            boost::thread t2( boost::bind( fn_migrate_fibers, ds, &b, &count) );

            t1.join();
            t2.join();

            fprintf(stderr, ", %d fibers stolen\n", count);
            delete ds;
        }

        return EXIT_SUCCESS;
    }
    catch ( std::exception const& e)
    { std::cerr << "exception: " << e.what() << std::endl; }
    catch (...)
    { std::cerr << "unhandled exception" << std::endl; }
    return EXIT_FAILURE;
}
