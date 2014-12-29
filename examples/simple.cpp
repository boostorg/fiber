#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>

#include <boost/bind.hpp>
#include <boost/intrusive_ptr.hpp>

#include <boost/fiber/all.hpp>

inline
void fn( std::string const& str, int n)
{
	for ( int i = 0; i < n; ++i)
	{
		std::cout << i << ": " << str << std::endl;
		boost::this_fiber::yield();
	}
}

void foo() {
    try
    {
        boost::fibers::fiber f1( boost::bind( fn, "abc", 0) );
        std::cerr << "f1 : " << f1.get_id() << std::endl;
        boost::fibers::fiber f2( boost::bind( fn, "xyz", 0) );
        std::cerr << "f2 : " << f2.get_id() << std::endl;

        f1.join();
        f2.join();
    }
	catch ( std::exception const& e)
	{ std::cerr << "exception: " << e.what() << std::endl; }
	catch (...)
	{ std::cerr << "unhandled exception" << std::endl; }
}

int main()
{
    try
    {
        std::thread( foo).join();

        std::cout << "done." << std::endl;

        return EXIT_SUCCESS;
    }
	catch ( std::exception const& e)
	{ std::cerr << "exception: " << e.what() << std::endl; }
	catch (...)
	{ std::cerr << "unhandled exception" << std::endl; }
	return EXIT_FAILURE;
}
