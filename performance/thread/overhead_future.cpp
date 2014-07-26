
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

#include <boost/chrono.hpp>
#include <boost/cstdint.hpp>
#include <boost/preprocessor.hpp>
#include <boost/program_options.hpp>
#include <boost/thread.hpp>
#include <boost/thread/future.hpp>

#include "../clock.hpp"

#ifndef JOBS
#define JOBS BOOST_PP_LIMIT_REPEAT
#endif

void worker() {}

void test_future()
{
    boost::packaged_task< void > pt( worker);
    boost::unique_future< void > f( pt.get_future() );
    boost::thread( boost::move( pt) ).detach();
    f.wait();
}

duration_type measure( duration_type overhead)
{
    test_future();

    time_point_type start( clock_type::now() );
    for ( std::size_t i = 0; i < JOBS; ++i) {
        test_future();
    }
    duration_type total = clock_type::now() - start;
    total -= overhead_clock(); // overhead of measurement
    total /= JOBS;  // loops

    return total;
}

int main( int argc, char * argv[])
{
    try
    {
        duration_type overhead = overhead_clock();
        std::cout << "overhead " << overhead.count() << " nano seconds" << std::endl;
        boost::uint64_t res = measure( overhead).count();
        std::cout << JOBS << " jobs: average of " << res << " nano seconds" << std::endl;

        return EXIT_SUCCESS;
    }
    catch ( std::exception const& e)
    { std::cerr << "exception: " << e.what() << std::endl; }
    catch (...)
    { std::cerr << "unhandled exception" << std::endl; }
    return EXIT_FAILURE;
}
