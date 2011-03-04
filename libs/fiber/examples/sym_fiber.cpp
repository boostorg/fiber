#include <cstdlib>
#include <iostream>
#include <string>

#include <boost/bind.hpp>

#include <boost/fiber/all.hpp>

int value = 0;
boost::sym_fiber gf1;
boost::sym_fiber gf2;

inline
void fn( std::string const& str, int n)
{
    for ( int i = 0; i < n; ++i)
    {
        std::cout << "symmetric fiber " << gf2.get_id() << ": increment value from " << value << " to ";
        ++value;
        std::cout << value << std::endl;
        gf2.switch_to( gf1);
    }
}

int main()
{
    try
    {
        int n = 5;
        gf1 = boost::sym_fiber::from_current_context();
        gf2 = boost::sym_fiber( fn, "abc", n, boost::sym_fiber::default_stacksize, gf1);
        std::cout << "start" << std::endl;

        while ( ! gf2.finished() )
        {
            gf1.switch_to( gf2);
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
