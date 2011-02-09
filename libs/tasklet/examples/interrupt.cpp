#include <cstdlib>
#include <iostream>
#include <string>

#include <boost/bind.hpp>

#include <boost/tasklet.hpp>

int value1 = 0;
int value2 = 0;
int value3 = 0;

void fn_1()
{
	for ( int i = 0; i < 10; ++i)
	{
		++value1;
		std::cout << "fn_1() increment value1 " << value1 << std::endl;
		boost::this_tasklet::interruption_point();
		boost::this_tasklet::yield();
	}
}

void fn_2()
{
	boost::this_tasklet::disable_interruption disabled;
	for ( int i = 0; i < 10; ++i)
	{
		++value2;
		std::cout << "fn_2() increment value2 " << value2 << std::endl;
		boost::this_tasklet::interruption_point();
		boost::this_tasklet::yield();
	}
}

void fn_3()
{
	boost::this_tasklet::disable_interruption disabled;
	for ( int i = 0; i < 10; ++i)
	{
		++value3;
		std::cout << "fn_3() increment value3 " << value3 << std::endl;
		boost::this_tasklet::restore_interruption restored( disabled);
		boost::this_tasklet::interruption_point();
		boost::this_tasklet::yield();
	}
}

void fn_4( boost::tasklet f)
{
	for ( int i = 0; i < 10; ++i)
	{
		if ( i == 1)
		{
			std::cout << "fn_4() interrupt tasklet " << f.get_id() << std::endl;
			f.interrupt();
			break;
		}
		boost::this_tasklet::yield();
	}
}

int main()
{
	try
	{
		boost::tasklets::scheduler<> sched;

		boost::tasklet f1( fn_1, boost::tasklet::default_stacksize, boost::protected_stack_allocator());
		sched.submit_tasklet( f1);
		sched.submit_tasklet(
			boost::tasklet(
				fn_4, f1, boost::tasklet::default_stacksize, boost::protected_stack_allocator()) );

		std::cout << "start: interrupt fn_1()" << std::endl;

		for (;;)
		{
			while ( sched.run() );
			if ( sched.empty() ) break;
		}

		std::cout << "finish: value1 == " << value1 << std::endl;

		boost::tasklet f2( fn_2, boost::tasklet::default_stacksize, boost::protected_stack_allocator());
		sched.submit_tasklet( f2);
		sched.submit_tasklet(
			boost::tasklet(
				fn_4, f2, boost::tasklet::default_stacksize, boost::protected_stack_allocator()) );
		std::cout << "start: interrupt fn_2()" << std::endl;

		for (;;)
		{
			while ( sched.run() );
			if ( sched.empty() ) break;
		}

		std::cout << "finish: value2 == " << value2 << std::endl;

		boost::tasklet f3( fn_3, boost::tasklet::default_stacksize, boost::protected_stack_allocator());
		sched.submit_tasklet( f3);
		sched.submit_tasklet(
			boost::tasklet(
				fn_4, f3, boost::tasklet::default_stacksize, boost::protected_stack_allocator()) );
		std::cout << "start: interrupt fn_3()" << std::endl;

		for (;;)
		{
			while ( sched.run() );
			if ( sched.empty() ) break;
		}

		std::cout << "finish: value3 == " << value3 << std::endl;

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
