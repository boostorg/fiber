#include <cstdlib>
#include <iostream>

#include <boost/fiber/all.hpp>

int main()
{
	try
	{
		std::cout << "max stack-size: " << boost::asym_fiber::max_stacksize / 1024 << " kB\n";
		std::cout << "min stack-size: " << boost::asym_fiber::min_stacksize / 1024 << " kB\n";
		std::cout << "default stack-size: " << boost::asym_fiber::default_stacksize / 1024 << " kB\n";

		return EXIT_SUCCESS;
	}
	catch ( std::exception const& e)
	{ std::cerr << "exception: " << e.what() << std::endl; }
	catch (...)
	{ std::cerr << "unhandled exception" << std::endl; }
	return EXIT_FAILURE;
}
