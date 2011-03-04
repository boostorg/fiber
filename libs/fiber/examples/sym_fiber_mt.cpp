#include <cstdlib>
#include <iostream>
#include <string>
#include <sstream>

#include <boost/bind.hpp>
#include <boost/thread.hpp>

#include <boost/fiber/all.hpp>

int value = 0;
boost::sym_fiber gf1;
boost::sym_fiber gf2;

void increment_value_fn()
{
    while ( true)
    {
        std::stringstream ss;
        ss << boost::this_thread::get_id();
        std::cout << "thread " << ss.str() << ": increment value from " << value << " to ";
        ++value;
        std::cout << value << std::endl;
        gf2.switch_to( gf1);
    }
}

void increment( int k, int n)
{
    gf1 = boost::sym_fiber::from_current_context();
    for ( int i = k; i < n; ++i)
        gf1.switch_to( gf2);
}

void fn_first( int k, int n, boost::barrier & b)
{
    std::stringstream ss;
    ss << boost::this_thread::get_id();
    std::cout << "thread " << ss.str() << " executes fiber " << gf2.get_id() << std::endl;
    increment( k, n);
    b.wait();
}

void fn_last( int k, int n, boost::barrier & b)
{
    b.wait();
    std::stringstream ss;
    ss << boost::this_thread::get_id();
    std::cout << "thread " << ss.str() << " executes fiber " << gf2.get_id() << std::endl;
    increment( k, n);
}

int main()
{
    try
    {
        std::cout << "start" << std::endl;

        value = 0;

        gf2 = boost::sym_fiber( increment_value_fn, boost::sym_fiber::default_stacksize);

        boost::barrier b( 2);
        boost::thread t1( fn_first, 0, 5, boost::ref( b) );
        boost::thread t2( fn_last, 5, 8, boost::ref( b) );
        t1.join();
        t2.join();

        std::cout << "finish" << std::endl;

        return EXIT_SUCCESS;
    }
    catch ( std::exception const& e)
    { std::cerr << "exception: " << e.what() << std::endl; }
    catch (...)
    { std::cerr << "unhandled exception" << std::endl; }
    return EXIT_FAILURE;
}
