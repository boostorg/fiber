
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

#ifndef JOBS
#define JOBS BOOST_PP_LIMIT_REPEAT
#endif

#define JOIN(z, n, _) \
    boost::fibers::fiber( attrs, worker).join();

boost::coroutines::flag_fpu_t preserve_fpu_ = boost::coroutines::fpu_not_preserved;
boost::coroutines::flag_unwind_t unwind_stack = boost::coroutines::no_stack_unwind;
bool prealloc = false;
bool preserve = false;
bool unwind = false;

void worker()
{ boost::this_fiber::yield(); }

duration_type measure( duration_type overhead)
{
    boost::fibers::attributes attrs( unwind_stack, preserve_fpu_);
    boost::fibers::fiber( attrs, worker).join();

    time_point_type start( clock_type::now() );

    BOOST_PP_REPEAT_FROM_TO(1, JOBS, JOIN, _)

    duration_type total = clock_type::now() - start;
    total -= overhead_clock(); // overhead of measurement
    total /= JOBS;  // loops

    return total;
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

        if ( preserve) preserve_fpu_ = boost::coroutines::fpu_preserved;
        if ( unwind) unwind_stack = boost::coroutines::stack_unwind;

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
