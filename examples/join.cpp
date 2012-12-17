#include <cstdlib>
#include <iostream>
#include <string>

#include <boost/bind.hpp>

#include <boost/fiber/all.hpp>

namespace stm = boost::fibers;
namespace this_stm = boost::this_fiber;

int value1 = 0;
int value2 = 0;

void fn1()
{
    stm::fiber::id id = this_stm::get_id();
    for ( int i = 0; i < 5; ++i)
    {
        ++value1;
        std::cout << "fiber " << id << " fn1: increment value1: " << value1 << std::endl;
        this_stm::yield();
    }
    std::cout << "fiber " << id << " fn1: returns" << std::endl;
}

void fn2( stm::fiber & s)
{
    stm::fiber::id id = this_stm::get_id();
    for ( int i = 0; i < 5; ++i)
    {
        ++value2;
        std::cout << "fiber " << id << " fn2: increment value2: " << value2 << std::endl;
        if ( i == 1)
        {
            std::cout << "fiber " << id << " fn2: joins fiber " << s.get_id() << std::endl;
            s.join();
            std::cout << "fiber " << id << " fn2: joined fiber " << s.get_id() << std::endl;
        }
        this_stm::yield();
    }
    std::cout << "fiber " << id << " fn2: returns" << std::endl;
}

int main()
{
    stm::default_scheduler ds;
    stm::scheduler::replace( & ds);
    try
    {
        stm::fiber s1( fn1);
        stm::fiber s2( boost::bind( fn2, boost::ref( s1) ) );

        s1.join();
        s2.join();

        std::cout << "done." << std::endl;

        return EXIT_SUCCESS;
    }
    catch ( std::exception const& e)
    { std::cerr << "exception: " << e.what() << std::endl; }
    catch (...)
    { std::cerr << "unhandled exception" << std::endl; }
    return EXIT_FAILURE;
}
