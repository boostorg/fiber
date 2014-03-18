
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

#include <tbb/task.h>
#include <tbb/task_scheduler_init.h>

#include "../bind_processor.hpp"
#include "../clock.hpp"

boost::uint64_t jobs = 1000;

struct worker: public tbb::task
{
    tbb::task * execute()
    {
        set_ref_count(  1);
        return 0;
    }
};

void test_enqueue()
{
    tbb::task * t = new ( tbb::task::allocate_root() ) worker();
    tbb::task::enqueue( * t);
    t->wait_for_all();
}

duration_type measure( duration_type overhead)
{
    test_enqueue();

    time_point_type start( clock_type::now() );
    for ( std::size_t i = 0; i < jobs; ++i) {
        test_enqueue();
    }
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

        tbb::task_scheduler_init init( 1);

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
