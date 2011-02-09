#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

#include <boost/tasklet.hpp>
#include <boost/tasklet/future.hpp>

class callable
{
private:
	struct impl
	{
		virtual ~impl() {}

		virtual void exec() = 0;
	};

	template< typename T >
	class timpl : public impl
	{
	private:
		boost::tasklets::packaged_task< T >	pt_;

	public:
		timpl( boost::tasklets::packaged_task< T > & pt) :
			pt_( boost::move( pt) )
		{}

		void exec()
		{ pt_(); }
	};

	boost::shared_ptr< impl >	impl_;

public:
	template< typename T >
	callable( boost::tasklets::packaged_task< T > & pt) :
		impl_( new timpl< T >( pt) )
	{}

	void operator()()
	{ impl_->exec(); }
};

int value1 = 0;
int value2 = 0;

inline
std::string helloworld_fn()
{
	std::stringstream ss;
	ss << boost::this_thread::get_id();

	fprintf( stderr, "thread %s returns 'Hello World'\n", ss.str().c_str() );
	return "Hello World";
}

void increment_fn1()
{
	std::stringstream ss;
	ss << boost::this_thread::get_id();

	for ( int i = 0; i < 5; ++i)
	{
		++value1;
		fprintf( stderr, "thread %s incremented value1 %d\n", ss.str().c_str(), value1);
		boost::this_tasklet::yield();
	}
}

void increment_fn2()
{
	std::stringstream ss;
	ss << boost::this_thread::get_id();

	for ( int i = 0; i < 3; ++i)
	{
		++value2;
		fprintf( stderr, "thread %s incremented value2 %d\n", ss.str().c_str(), value2);
	}

	++value2;
	fprintf( stderr, "thread %s incremented value2 %d\n", ss.str().c_str(), value2);
	boost::this_tasklet::yield();
	++value2;
	fprintf( stderr, "thread %s incremented value2 %d\n", ss.str().c_str(), value2);
	boost::this_tasklet::yield();
}

void waiting_fn( boost::tasklets::shared_future< std::string > fu)
{
	std::stringstream ss;
	ss << boost::this_thread::get_id();

	fprintf( stderr, "thread %s waits for future\n", ss.str().c_str() );
	std::string result = fu.get();
	fprintf( stderr, "thread %s got string %s from future\n", ss.str().c_str(), result.c_str() );
}

void thread_fn1( boost::tasklets::shared_future< std::string > fu)
{
	std::stringstream ss;
	ss << boost::this_thread::get_id();

	fprintf( stderr, "thread %s started\n", ss.str().c_str() );

	boost::tasklets::scheduler<> sched;
	
	sched.submit_tasklet(
		boost::tasklet(
			& increment_fn1, boost::tasklet::default_stacksize, boost::protected_stack_allocator()) );
	sched.submit_tasklet(
		boost::tasklet(
			& waiting_fn, fu, boost::tasklet::default_stacksize, boost::protected_stack_allocator()) );
	
	for (;;)
	{
		while ( sched.run() );
		if ( sched.empty() ) break;
	}
	fprintf( stderr, "thread %s finished\n", ss.str().c_str() );
}

void thread_fn2( callable ca)
{
	std::stringstream ss;
	ss << boost::this_thread::get_id();

	fprintf( stderr, "thread %s started\n", ss.str().c_str() );

	boost::tasklets::scheduler<> sched;

	sched.submit_tasklet(
		boost::tasklet(
			& increment_fn2, boost::tasklet::default_stacksize, boost::protected_stack_allocator()) );
	sched.submit_tasklet(
		boost::tasklet(
			ca, boost::tasklet::default_stacksize, boost::protected_stack_allocator()) );

	for (;;)
	{
		while ( sched.run() );
		if ( sched.empty() ) break;
	}
	fprintf( stderr, "thread %s finished\n", ss.str().c_str() );
}

int main()
{
	try
	{
		boost::tasklets::packaged_task< std::string > pt( helloworld_fn);
		boost::tasklets::unique_future< std::string > fu = pt.get_future();

		boost::thread th1( boost::bind( & thread_fn1, boost::tasklets::shared_future< std::string >( fu) ) );
		boost::thread th2( boost::bind( & thread_fn2, callable( pt) ) );

		th1.join();
		th2.join();

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
