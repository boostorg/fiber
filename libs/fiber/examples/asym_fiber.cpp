#include <cstdlib>
#include <iostream>
#include <string>

#include <boost/bind.hpp>

#include <boost/fiber/all.hpp>

int value = 0;
boost::asym_fiber gf;

inline
void fn( std::string const& str, int n)
{
    for ( int i = 0; i < n; ++i)
    {
        std::cout << "asymmetric fiber " << gf.get_id() << ": increment value from " << value << " to ";
        ++value;
        std::cout << value << std::endl;
        gf.yield();
    }
}

int main()
{
    try
    {
        int n = 5;
        gf = boost::asym_fiber( fn, "abc", n, boost::asym_fiber::default_stacksize);
        std::cout << "start" << std::endl;

        while ( ! gf.finished() )
        {
            gf.run();
            std::cout << value << " iteration" << std::endl;
        }

        std::cout << "finish" << std::endl;

        return EXIT_SUCCESS;
    }
    catch ( std::exception const& e)
    { std::cerr << "exception: " << e.what() << std::endl; }
    catch (...)
    { std::cerr << "unhandled exception" << std::endl; }
    return EXIT_FAILURE;
}
