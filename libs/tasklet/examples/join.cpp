#include <cstdlib>
#include <iostream>
#include <string>

#include <boost/bind.hpp>

#include <boost/tasklet.hpp>

int value1 = 0;
int value2 = 0;

void fn_1()
{
	for ( int i = 0; i < 5; ++i)
	{
		++value1;
		std::cout << "fn_1() increment value1 " << value1 << std::endl;
		boost::this_tasklet::yield();
	}
}

void fn_2( boost::tasklet f)
{
	for ( int i = 0; i < 5; ++i)
	{
		++value2;
		std::cout << "fn_2() increment value2 " << value2 << std::endl;
		if ( i == 1)
		{
			std::cout << "fn_2() join tasklet " << f.get_id() << std::endl;
			f.join();
			std::cout << "fn_2() tasklet " << f.get_id() << " joined" << std::endl;
		}
		boost::this_tasklet::yield();
	}
}

int main()
{
	try
	{
		boost::tasklets::scheduler<> sched;

		boost::tasklet f( fn_1, boost::tasklet::default_stacksize, boost::protected_stack_allocator());
		sched.submit_tasklet( f);
		sched.submit_tasklet(
			boost::tasklet(
				fn_2, f, boost::tasklet::default_stacksize, boost::protected_stack_allocator()) );

		std::cout << "start" << std::endl;
		std::cout << "tasklet to be joined " << f.get_id() << std::endl;

		for (;;)
		{
			while ( sched.run() );
			if ( sched.empty() ) break;
		}

		std::cout << "finish: value1 == " << value1 << ", value2 == " << value2 << std::endl;

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
