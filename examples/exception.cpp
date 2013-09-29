#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

#include <boost/bind.hpp>

#include <boost/fiber/all.hpp>

void throw_exception()
{
    boost::this_fiber::yield();
    throw std::runtime_error("exception in fiber");
}

int main()
{
    boost::fibers::round_robin ds;
    boost::fibers::set_scheduling_algorithm( & ds);

    try
    {
        boost::fibers::fiber f(throw_exception);
        f.join();

        std::cout << "done." << std::endl;

        return EXIT_SUCCESS;
    }
    catch ( boost::fibers::fiber_interrupted const&)
    { std::cerr << "interrupted" << std::endl; }
    catch ( std::exception const& e)
    { std::cerr << "exception: " << e.what() << std::endl; }
    catch (...)
    { std::cerr << "unhandled exception" << std::endl; }

    return EXIT_FAILURE;
}
