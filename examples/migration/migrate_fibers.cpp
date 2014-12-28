
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <atomic>
#include <cstdio>
#include <sstream>
#include <string>
#include <thread>

#include <boost/assert.hpp>

#include <boost/fiber/all.hpp>
#include "workstealing_round_robin.hpp"

#define MAXCOUNT 10

std::atomic< bool > fini( false);

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
    boost::fibers::packaged_task< int() > pt( std::bind( fibonacci_, n) );
    boost::fibers::future< int > f( pt.get_future() );
    boost::fibers::fiber( boost::fibers::fixedsize(1024*1024), std::move( pt) ).detach();
    return std::move( f);
}

int create_fiber( int n)
{
    return fibonacci( n).get();
}

void fn_create_fibers( workstealing_round_robin * ds)
{
    boost::fibers::set_scheduling_algorithm( ds);

    int n = 10;
    int result = boost::fibers::async( boost::fibers::fixedsize(1024*1024),
                                       std::bind( create_fiber, n) ).get();
    BOOST_ASSERT( 89 == result);
    fprintf( stderr, "fibonacci(%d) = %d", n, result);

    fini = true;
}

void fn_migrate_fibers( workstealing_round_robin * other_ds, int * count)
{
    BOOST_ASSERT( other_ds);

    while ( ! fini)
    {
        // To guarantee progress, we must ensure that
        // threads that have work to do are not unreasonably delayed by (thief) threads
        // which are idle except for task-stealing. 
        // This call yields the thief ’s processor to another thread, allowing
        // descheduled threads to regain a processor and make progress. 
        std::this_thread::yield();

        boost::fibers::fiber f( other_ds->steal() );
        if ( f)
        {
            ++( * count);
            boost::fibers::migrate( f);
            f.join();
        }
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
            std::thread t1( std::bind( fn_create_fibers, ds) );
            std::thread t2( std::bind( fn_migrate_fibers, ds, &count) );

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
