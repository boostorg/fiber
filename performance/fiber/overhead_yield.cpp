
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

#include <boost/chrono.hpp>
#include <boost/fiber/all.hpp>
#include <boost/cstdint.hpp>
#include <boost/preprocessor.hpp>
#include <boost/program_options.hpp>
#include <boost/thread/detail/memory.hpp> // boost::allocator_arg_t

#include "../clock.hpp"
#include "../preallocated_stack_allocator.hpp"

#ifndef JOBS
#define JOBS BOOST_PP_LIMIT_REPEAT
#endif

#define JOIN(z, n, _) \
    boost::fibers::fiber(boost::allocator_arg, stack_alloc, attrs, worker).join();

boost::coroutines::flag_fpu_t preserve_fpu = boost::coroutines::fpu_not_preserved;
boost::coroutines::flag_unwind_t unwind_stack = boost::coroutines::no_stack_unwind;
bool prealloc = false;
bool preserve = false;
bool unwind = false;

void worker()
{ boost::this_fiber::yield(); }

template< typename StackAllocator >
duration_type measure( duration_type overhead, StackAllocator const& stack_alloc)
{
    boost::fibers::attributes attrs( unwind_stack, preserve_fpu);
    boost::fibers::fiber( boost::allocator_arg, stack_alloc, attrs, worker).join();

    time_point_type start( clock_type::now() );

    BOOST_PP_REPEAT_FROM_TO(1, JOBS, JOIN, _)

    duration_type total = clock_type::now() - start;
    total -= overhead_clock(); // overhead of measurement
    total /= JOBS;  // loops

    return total;
}

duration_type measure_standard( duration_type overhead)
{
    boost::fibers::stack_allocator stack_alloc;

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
            ("fpu,f", boost::program_options::value< bool >( & preserve), "preserve FPU registers")
            ("unwind,u", boost::program_options::value< bool >( & unwind), "unwind fiber-stack")
            ("prealloc,p", boost::program_options::value< bool >( & prealloc), "use preallocated stack");

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

        if ( preserve) preserve_fpu = boost::coroutines::fpu_preserved;
        if ( unwind) unwind_stack = boost::coroutines::stack_unwind;

        duration_type overhead = overhead_clock();
        std::cout << "overhead " << overhead.count() << " nano seconds" << std::endl;
        boost::uint64_t res =
            prealloc
            ? measure_prealloc( overhead).count()
            : measure_standard( overhead).count();
        std::cout << JOBS << " jobs: average of " << res << " nano seconds" << std::endl;

        return EXIT_SUCCESS;
    }
    catch ( std::exception const& e)
    { std::cerr << "exception: " << e.what() << std::endl; }
    catch (...)
    { std::cerr << "unhandled exception" << std::endl; }
    return EXIT_FAILURE;
}
