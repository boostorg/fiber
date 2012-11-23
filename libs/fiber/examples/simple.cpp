#include <cstdlib>
#include <iostream>
#include <string>

#include <boost/bind.hpp>

#include <boost/fiber/all.hpp>

namespace stm = boost::fibers;
namespace this_stm = boost::this_fiber;

inline
void fn( std::string const& str, int n)
{
	for ( int i = 0; i < n; ++i)
	{
		std::cout << i << ": " << str << std::endl;
		this_stm::yield();
	}
}

int main()
{
	try
	{
        stm::fiber s1( boost::bind( fn, "abc", 5) );
        stm::fiber s2( boost::bind( fn, "xyz", 7) );

		while ( s1 || s2 ) stm::run();

		std::cout << "done." << std::endl;

		return EXIT_SUCCESS;
	}
	catch ( std::exception const& e)
	{ std::cerr << "exception: " << e.what() << std::endl; }
	catch (...)
	{ std::cerr << "unhandled exception" << std::endl; }
	return EXIT_FAILURE;
}
