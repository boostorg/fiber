#include <cstdlib>
#include <iostream>

#include <boost/assert.hpp>
#include <boost/ref.hpp>
#include <boost/thread.hpp>

#include <boost/fiber/all.hpp>

int value1 = 0;
int value2 = 0;

inline
void fn1( boost::fibers::barrier & b)
{
    boost::fibers::fiber::id id = boost::this_fiber::get_id();
	std::cout << "fiber " << id << ": fn1 entered" << std::endl;

	++value1;
	std::cout << "fiber " << id << ": incremented value1: " << value1 << std::endl;
	boost::this_fiber::yield();

	std::cout << "fiber " << id << ": waits for barrier" << std::endl;
	b.wait();
	std::cout << "fiber " << id << ": passed barrier" << std::endl;

	++value1;
	std::cout << "fiber " << id << ": incremented value1: " << value1 << std::endl;
	boost::this_fiber::yield();

	++value1;
	std::cout << "fiber " << id << ": incremented value1: " << value1 << std::endl;
	boost::this_fiber::yield();

	++value1;
	std::cout << "fiber " << id << ": incremented value1: " << value1 << std::endl;
	boost::this_fiber::yield();

	std::cout << "fiber " << id << ": fn1 returns" << std::endl;
}

inline
void fn2( boost::fibers::barrier & b)
{
    boost::fibers::fiber::id id = boost::this_fiber::get_id();
	std::cout << "fiber " << id << ": fn2 entered" << std::endl;

	++value2;
	std::cout << "fiber " << id << ": incremented value2: " << value2 << std::endl;
	boost::this_fiber::yield();

	++value2;
	std::cout << "fiber " << id << ": incremented value2: " << value2 << std::endl;
	boost::this_fiber::yield();

	++value2;
	std::cout << "fiber " << id << ": incremented value2: " << value2 << std::endl;
	boost::this_fiber::yield();

	std::cout << "fiber " << id << ": waits for barrier" << std::endl;
	b.wait();
	std::cout << "fiber " << id << ": passed barrier" << std::endl;

	++value2;
	std::cout << "fiber " << id << ": incremented value2: " << value2 << std::endl;
	boost::this_fiber::yield();

	std::cout << "fiber " << id << ": fn2 returns" << std::endl;
}

int main()
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);

	try
	{
		boost::fibers::barrier fb( 2);

		boost::fibers::fiber s1( boost::bind( & fn1, boost::ref( fb) ) );
		boost::fibers::fiber s2( boost::bind( & fn2, boost::ref( fb) ) );

		while ( s1 || s2)
			boost::fibers::run();

		std::cout << "done." << std::endl;

		return EXIT_SUCCESS;
	}
	catch ( std::exception const& e)
	{ std::cerr << "exception: " << e.what() << std::endl; }
	catch (...)
	{ std::cerr << "unhandled exception" << std::endl; }
	return EXIT_FAILURE;
}
