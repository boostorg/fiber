
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

#include <boost/atomic.hpp>
#include <boost/chrono.hpp>
#include <boost/cstdint.hpp>
#include <boost/program_options.hpp>

#include <qthread/qthread.h>

#include "../bind_processor.hpp"
#include "../clock.hpp"

boost::uint64_t jobs = 1000;

boost::atomic< boost::uint64_t > counter( 0);

extern "C" aligned_t worker( void *)
{
    ++counter;
    return aligned_t();
}

duration_type measure( duration_type overhead)
{
    time_point_type start( clock_type::now() );
    for ( std::size_t i = 0; i < jobs; ++i) {
        qthread_fork( & worker, 0, 0);
    }
    do
    {
        qthread_yield();
    } while ( counter != jobs);
    duration_type total = clock_type::now() - start;
    total -= overhead_clock(); // overhead of measurement
    total /= jobs;  // loops

    return total;
}

int main( int argc, char * argv[])
{
    try
    {
        bool preserve = false, bind = false, unwind = false;
        boost::program_options::options_description desc("allowed options");
        desc.add_options()
            ("help", "help message")
            ("bind,b", boost::program_options::value< bool >( & bind), "bind thread to CPU")
            ("fpu,f", boost::program_options::value< bool >( & preserve), "preserve FPU registers")
            ("unwind,u", boost::program_options::value< bool >( & unwind), "unwind fiber-stack")
            ("jobs,j", boost::program_options::value< boost::uint64_t >( & jobs), "jobs to run");

        boost::program_options::variables_map vm;
        boost::program_options::store(
                boost::program_options::parse_command_line(
                    argc,
                    argv,
                    desc),
                vm);
        boost::program_options::notify( vm);

        if ( vm.count("help") ) {
            std::cout << desc << std::endl;
            return EXIT_SUCCESS;
        }

        if ( bind) bind_to_processor( 0);

        setenv("QT_NUM_SHEPHERDS", "1", 1);
        setenv("QT_NUM_WORKERS_PER_SHEPHERD", "1", 1);

        // Setup the qthreads environment.
        if ( 0 != qthread_initialize() )
            throw std::runtime_error("qthreads failed to initialize\n");

        duration_type overhead = overhead_clock();
        std::cout << "overhead " << overhead.count() << " nano seconds" << std::endl;
        boost::uint64_t res = measure( overhead).count();
        std::cout << "average of " << res << " nano seconds" << std::endl;

        return EXIT_SUCCESS;
    }
    catch ( std::exception const& e)
    { std::cerr << "exception: " << e.what() << std::endl; }
    catch (...)
    { std::cerr << "unhandled exception" << std::endl; }
    return EXIT_FAILURE;
}
