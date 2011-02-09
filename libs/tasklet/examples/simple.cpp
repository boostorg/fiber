#include <cstdlib>
#include <iostream>
#include <string>

#include <boost/bind.hpp>

#include <boost/tasklet.hpp>

inline
void fn( std::string const& str, int n)
{
	for ( int i = 0; i < n; ++i)
	{
		std::cout << i << ": " << str << std::endl;
		boost::this_tasklet::yield();
	}
}

int main()
{
	try
	{
		boost::tasklets::scheduler<> sched;

		boost::tasklet f( fn, "abc", 5, boost::tasklet::default_stacksize, boost::protected_stack_allocator() );
		sched.submit_tasklet( boost::move( f) );
		sched.submit_tasklet(
			boost::tasklet(
				& fn, "xyz", 7, boost::tasklet::default_stacksize, boost::protected_stack_allocator() ) );

		std::cout << "start" << std::endl;

		for (;;)
		{
			while ( sched.run() );
			if ( sched.empty() ) break;
		}

		std::cout << "finish" << std::endl;

		return EXIT_SUCCESS;
	}
	catch ( boost::tasklets::scheduler_error const& e)
	{ std::cerr << "scheduler_error: " << e.what() << std::endl; }
	catch ( std::exception const& e)
	{ std::cerr << "exception: " << e.what() << std::endl; }
	catch (...)
	{ std::cerr << "unhandled exception" << std::endl; }
	return EXIT_FAILURE;
}
