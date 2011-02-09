#include <cstdlib>
#include <iostream>
#include <string>

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

#include <boost/tasklet.hpp>
#include <boost/tasklet/future.hpp>

inline
std::string helloworld_fn()
{ return "Hello World"; }

int main()
{
	try
	{
		boost::tasklets::scheduler<> sched;

		boost::tasklets::packaged_task< std::string > pt( helloworld_fn);
		boost::tasklets::unique_future< std::string > fu = pt.get_future();
		boost::tasklet t( boost::move( pt), boost::tasklet::default_stacksize, boost::protected_stack_allocator());
		sched.submit_tasklet( t);

		for (;;)
		{
			while ( sched.run() );
			if ( sched.empty() ) break;
		}

		std::cout << fu.get() << std::endl;

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
