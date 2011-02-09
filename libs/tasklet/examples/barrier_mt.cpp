#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <string>
#include <sstream>

#include <boost/assert.hpp>
#include <boost/ref.hpp>
#include <boost/thread.hpp>

#include <boost/tasklet.hpp>

int value1 = 0;
int value2 = 0;

inline
void fn1( boost::tasklets::barrier & b)
{
	std::stringstream tss;
	tss << boost::this_thread::get_id();
	std::stringstream fss;
	fss << boost::this_tasklet::get_id();

	fprintf( stderr, "start fn1 tasklet %s in thread %s\n", fss.str().c_str(), tss.str().c_str() );

	++value1;
	fprintf( stderr, "fn1 tasklet %s in thread %s increment %d\n", fss.str().c_str(), tss.str().c_str(), value1);

	fprintf( stderr, "fn1 tasklet %s in thread %s before wait for barrier\n", fss.str().c_str(), tss.str().c_str());
	b.wait();
	fprintf( stderr, "fn1 tasklet %s in thread %s after wait for barrier\n", fss.str().c_str(), tss.str().c_str());

	++value1;
	fprintf( stderr, "fn1 tasklet %s in thread %s increment %d\n", fss.str().c_str(), tss.str().c_str(), value1);

	++value1;
	fprintf( stderr, "fn1 tasklet %s in thread %s increment %d\n", fss.str().c_str(), tss.str().c_str(), value1);

	++value1;
	fprintf( stderr, "fn1 tasklet %s in thread %s increment %d\n", fss.str().c_str(), tss.str().c_str(), value1);

	++value1;
	fprintf( stderr, "fn1 tasklet %s in thread %s increment %d\n", fss.str().c_str(), tss.str().c_str(), value1);

	fprintf( stderr, "end fn1 tasklet %s in thread %s\n", fss.str().c_str(), tss.str().c_str() );
}

inline
void fn2( boost::tasklets::barrier & b)
{
	std::stringstream tss;
	tss << boost::this_thread::get_id();
	std::stringstream fss;
	fss << boost::this_tasklet::get_id();

	fprintf( stderr, "start fn2 tasklet %s in thread %s\n", fss.str().c_str(), tss.str().c_str() );

	++value2;
	fprintf( stderr, "fn2 tasklet %s in thread %s increment %d\n", fss.str().c_str(), tss.str().c_str(), value2);

	++value2;
	fprintf( stderr, "fn2 tasklet %s in thread %s increment %d\n", fss.str().c_str(), tss.str().c_str(), value2);

	++value2;
	fprintf( stderr, "fn2 tasklet %s in thread %s increment %d\n", fss.str().c_str(), tss.str().c_str(), value2);

	++value2;
	fprintf( stderr, "fn2 tasklet %s in thread %s increment %d\n", fss.str().c_str(), tss.str().c_str(), value2);

	fprintf( stderr, "fn2 tasklet %s in thread %s before wait for barrier\n", fss.str().c_str(), tss.str().c_str());
	b.wait();
	fprintf( stderr, "fn2 tasklet %s in thread %s after wait for barrier\n", fss.str().c_str(), tss.str().c_str());

	++value2;
	fprintf( stderr, "fn2 tasklet %s in thread %s increment %d\n", fss.str().c_str(), tss.str().c_str(), value2);

	fprintf( stderr, "end fn2 tasklet %s in thread %s\n", fss.str().c_str(), tss.str().c_str() );
}

void f(
	boost::barrier & tb,
	boost::tasklets::barrier & fb)
{
	std::stringstream tss;
	tss << boost::this_thread::get_id();

	fprintf( stderr, "start thread %s\n", tss.str().c_str() );

	boost::tasklets::scheduler<> sched;
	
	sched.submit_tasklet(
		boost::tasklet(
			& fn1, boost::ref( fb), boost::tasklet::default_stacksize, boost::protected_stack_allocator()) );

	tb.wait();

	for (;;)
	{
		while ( sched.run() );
		if ( sched.empty() ) break;
	}

	fprintf( stderr, "end thread %s\n", tss.str().c_str() );
}

void g(
	boost::barrier & tb,
	boost::tasklets::barrier & fb)
{
	std::stringstream tss;
	tss << boost::this_thread::get_id();

	fprintf( stderr, "start thread %s\n", tss.str().c_str() );

	boost::tasklets::scheduler<> sched;
	
	sched.submit_tasklet(
		boost::tasklet(
			& fn2, boost::ref( fb), boost::tasklet::default_stacksize, boost::protected_stack_allocator()) );

	tb.wait();

	for (;;)
	{
		while ( sched.run() );
		if ( sched.empty() ) break;
	}

	fprintf( stderr, "end thread %s\n", tss.str().c_str() );
}

int main()
{
	try
	{
		boost::barrier tb( 2);
		boost::tasklets::barrier fb( 2);

		std::cout << "start" << std::endl;

		boost::thread th1( boost::bind( & f, boost::ref( tb), boost::ref( fb) ) );
		boost::thread th2( boost::bind( & g, boost::ref( tb), boost::ref( fb) ) );

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
