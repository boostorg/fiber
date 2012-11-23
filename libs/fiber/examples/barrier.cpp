#include <cstdlib>
#include <iostream>

#include <boost/assert.hpp>
#include <boost/ref.hpp>
#include <boost/thread.hpp>

#include <boost/fiber/all.hpp>

namespace stm = boost::fibers;
namespace this_stm = boost::this_fiber;

int value1 = 0;
int value2 = 0;

inline
void fn1( stm::barrier & b)
{
    stm::fiber::id id = this_stm::get_id();
	std::cout << "fiber " << id << ": fn1 entered" << std::endl;

	++value1;
	std::cout << "fiber " << id << ": incremented value1: " << value1 << std::endl;
	this_stm::yield();

	std::cout << "fiber " << id << ": waits for barrier" << std::endl;
	b.wait();
	std::cout << "fiber " << id << ": passed barrier" << std::endl;

	++value1;
	std::cout << "fiber " << id << ": incremented value1: " << value1 << std::endl;
	this_stm::yield();

	++value1;
	std::cout << "fiber " << id << ": incremented value1: " << value1 << std::endl;
	this_stm::yield();

	++value1;
	std::cout << "fiber " << id << ": incremented value1: " << value1 << std::endl;
	this_stm::yield();

	std::cout << "fiber " << id << ": fn1 returns" << std::endl;
}

inline
void fn2( stm::barrier & b)
{
    stm::fiber::id id = this_stm::get_id();
	std::cout << "fiber " << id << ": fn2 entered" << std::endl;

	++value2;
	std::cout << "fiber " << id << ": incremented value2: " << value2 << std::endl;
	this_stm::yield();

	++value2;
	std::cout << "fiber " << id << ": incremented value2: " << value2 << std::endl;
	this_stm::yield();

	++value2;
	std::cout << "fiber " << id << ": incremented value2: " << value2 << std::endl;
	this_stm::yield();

	std::cout << "fiber " << id << ": waits for barrier" << std::endl;
	b.wait();
	std::cout << "fiber " << id << ": passed barrier" << std::endl;

	++value2;
	std::cout << "fiber " << id << ": incremented value2: " << value2 << std::endl;
	this_stm::yield();

	std::cout << "fiber " << id << ": fn2 returns" << std::endl;
}

int main()
{
	try
	{
		stm::barrier fb( 2);

		stm::fiber s1( boost::bind( & fn1, boost::ref( fb) ) );
		stm::fiber s2( boost::bind( & fn2, boost::ref( fb) ) );

		while ( s1 || s2)
			stm::run();

		std::cout << "done." << std::endl;

		return EXIT_SUCCESS;
	}
	catch ( std::exception const& e)
	{ std::cerr << "exception: " << e.what() << std::endl; }
	catch (...)
	{ std::cerr << "unhandled exception" << std::endl; }
	return EXIT_FAILURE;
}
