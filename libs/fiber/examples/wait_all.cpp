#include <cstdlib>
#include <iostream>
#include <string>

#include <boost/bind.hpp>
#include <boost/utility.hpp>

#include <boost/fiber/all.hpp>

namespace stm = boost::fibers;
namespace this_stm = boost::this_fiber;

struct X : private boost::noncopyable
{
    X()
    { std::cout << "fiber " << this_stm::get_id() << ": X()" << std::endl; }

    ~X()
    { std::cout << "fiber " << this_stm::get_id() << ": ~X()" << std::endl; }
};

inline
void fn( std::string const& str, int n)
{
    X x;

    stm::fiber::id id = this_stm::get_id();
	std::cout << "fiber " << id << ": fn1 entered" << std::endl;

	for ( int i = 0; i < n; ++i)
	{
	    std::cout << "fiber " << id << ": " << i << ", " << str << std::endl;
		this_stm::yield();
	}
	std::cout << "fiber " << id << ": fn returns" << std::endl;
}

int main()
{
	try
	{
        stm::fiber s1( stm::spawn( boost::bind( fn, "abc", 5) ) );
		stm::fiber s2( stm::spawn( boost::bind( fn, "xyz", 7) ) );

        stm::waitfor_all( s1, s2);

		std::cout << "done." << std::endl;

		return EXIT_SUCCESS;
	}
	catch ( std::exception const& e)
	{ std::cerr << "exception: " << e.what() << std::endl; }
	catch (...)
	{ std::cerr << "unhandled exception" << std::endl; }
	return EXIT_FAILURE;
}
