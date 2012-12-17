#include <cstdlib>
#include <iostream>
#include <string>

#include <boost/bind.hpp>

#include <boost/fiber/all.hpp>

namespace stm = boost::fibers;
namespace this_stm = boost::this_fiber;

inline
int fn( std::string const& str, int n)
{
	for ( int i = 0; i < n; ++i)
	{
		std::cout << i << ": " << str << std::endl;
		this_stm::yield();
	}

    return n;
}

void start()
{
    stm::packaged_task<int> pt(
        boost::bind( fn, "abc", 5) );
    stm::unique_future<int> fi=pt.get_future();
    stm::fiber( boost::move( pt) );
    fi.wait();
    std::cout << "fn() returned " << fi.get() << std::endl;
}

int main()
{
    stm::default_scheduler ds;
    stm::scheduler::replace( & ds);
	try
	{
        stm::fiber( start).join();
		std::cout << "done." << std::endl;

		return EXIT_SUCCESS;
	}
	catch ( std::exception const& e)
	{ std::cerr << "exception: " << e.what() << std::endl; }
	catch (...)
	{ std::cerr << "unhandled exception" << std::endl; }
	return EXIT_FAILURE;
}
