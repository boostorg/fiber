
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

#include <boost/fiber/all.hpp>
#include <boost/preprocessor.hpp>
#include <boost/program_options.hpp>

#include "../clock.hpp"
#include "../preallocated_stack_allocator.hpp"

#ifndef JOBS
#define JOBS BOOST_PP_LIMIT_REPEAT
#endif

bool prealloc = false;
bool preserve = false;

void worker() {}

template< typename StackAllocator >
void test_future( StackAllocator const& stack_alloc)
{
    boost::fibers::packaged_task< void() > pt( worker);
    boost::fibers::future< void > f( pt.get_future() );
    boost::fibers::fiber( stack_alloc, std::move( pt) ).detach();
    f.wait();
}

template< typename StackAllocator >
duration_type measure( duration_type overhead, StackAllocator const& stack_alloc)
{
    test_future( stack_alloc);

    time_point_type start( clock_type::now() );
    for ( std::size_t i = 0; i < JOBS; ++i) {
        test_future( stack_alloc);
    }
    duration_type total = clock_type::now() - start;
    total -= overhead_clock(); // overhead of measurement
    total /= JOBS;  // loops

    return total;
}

duration_type measure_standard( duration_type overhead)
{
    boost::fibers::fixedsize stack_alloc;

    return measure( overhead, stack_alloc);
}

duration_type measure_prealloc( duration_type overhead)
{
    preallocated_stack_allocator stack_alloc( JOBS + 1);

    return measure( overhead, stack_alloc);
}

int main( int argc, char * argv[])
{
    try
    {
        boost::program_options::options_description desc("allowed options");
        desc.add_options()
            ("help", "help message")
            ("preserve", boost::program_options::value< bool >( & preserve), "preserve FPU")
            ("prealloc", boost::program_options::value< bool >( & prealloc), "use preallocated stack");

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

        boost::fibers::preserve_fpu( preserve);

        duration_type overhead = overhead_clock();
        std::cout << "overhead " << overhead.count() << " nano seconds" << std::endl;
        boost::uint64_t res =
            prealloc
            ? measure_prealloc( overhead).count()
            : measure_standard( overhead).count();
        std::cout << "average of " << res << " nano seconds" << std::endl;

        return EXIT_SUCCESS;
    }
    catch ( std::exception const& e)
    { std::cerr << "exception: " << e.what() << std::endl; }
    catch (...)
    { std::cerr << "unhandled exception" << std::endl; }
    return EXIT_FAILURE;
}
