#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>

#include <boost/bind.hpp>
#include <boost/ref.hpp>
#include <boost/thread.hpp>

#include <boost/tasklet.hpp>

void g( std::string const& str, int n)
{
	for ( int i = 0; i < n; ++i)
	{
		std::ostringstream os1;
		std::ostringstream os2;
		os1	<< boost::this_thread::get_id();
		os2 << boost::this_tasklet::get_id();
		fprintf( stderr, "(thread: %s, tasklet: %s) %d: %s\n", os1.str().c_str(), os2.str().c_str(), i, str.c_str() );
		boost::this_tasklet::yield();
	}
}

void fn1(
		boost::tasklet & t,
		boost::barrier & b,
		boost::tasklets::scheduler<> & sched2,
		std::string const& msg, int n)
{
		std::ostringstream os;
		os << boost::this_thread::get_id();
		fprintf( stderr, "start (thread1: %s)\n", os.str().c_str() );

		boost::tasklets::scheduler<> sched1;
		sched1.submit_tasklet(
			boost::tasklet(
				& g, msg, n, boost::tasklet::default_stacksize, boost::protected_stack_allocator()) );

		for ( int i = 0; i < 2; ++i)
			sched1.run();

		b.wait();

		sched2.migrate_tasklet( t);

		b.wait();

		std::ostringstream id_os;
		id_os << t.get_id();

		fprintf( stderr, "thread1: tasklet %s migrated\n", id_os.str().c_str() );
		fprintf( stderr, "thread1: scheduler runs %d tasklets\n", static_cast< int >( sched1.size() ) );

		for (;;)
		{
			while ( sched1.run() );
			if ( sched1.empty() ) break;
		}

		fprintf( stderr, "finish (thread1: %s)\n", os.str().c_str() );
}

void fn2(
		boost::tasklet & f,
		boost::barrier & b,
		boost::tasklets::scheduler<> & sched)
{
		std::ostringstream os;
		os << boost::this_thread::get_id();
		fprintf( stderr, "start (thread2: %s)\n", os.str().c_str() );

		sched.submit_tasklet( f);

		sched.run();
		sched.run();

		b.wait();
		b.wait();

		fprintf( stderr, "thread2: scheduler runs %d tasklets\n", static_cast< int >( sched.size() ) );

		fprintf( stderr, "finish (thread2: %s)\n", os.str().c_str() );
}

int main()
{
	try
	{
		boost::tasklets::scheduler<> sched;
		boost::barrier b( 2);

		boost::tasklet f( & g, "xyz", 4, boost::tasklet::default_stacksize, boost::protected_stack_allocator());

		std::cout << "start" << std::endl;

		boost::thread th1(
				fn1,
				f,
				boost::ref( b),
				boost::ref( sched),
				"abc", 5);
		boost::thread th2(
				fn2,
				f,
				boost::ref( b),
				boost::ref( sched) );

		th1.join();
		th2.join();

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
