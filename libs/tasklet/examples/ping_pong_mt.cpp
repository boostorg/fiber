#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <string>
#include <sstream>


#include <boost/assert.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/ref.hpp>
#include <boost/optional.hpp>
#include <boost/thread.hpp>

#include <boost/tasklet.hpp>

typedef boost::tasklets::unbounded_channel< std::string >	fifo_t;

inline
void ping(
		fifo_t & recv_buf,
		fifo_t & send_buf)
{
	std::stringstream tss;
	tss << boost::this_thread::get_id();
	std::stringstream fss;
	fss << boost::this_tasklet::get_id();

	fprintf( stderr, "start ping tasklet %s in thread %s\n", fss.str().c_str(), tss.str().c_str() );

	boost::optional< std::string > value;

	send_buf.put("ping");
	BOOST_ASSERT( recv_buf.take( value) );
	fprintf( stderr, "ping tasklet %s in thread %s recevied %s\n", fss.str().c_str(), tss.str().c_str(), value->c_str());
	value.reset();

	send_buf.put("ping");
	BOOST_ASSERT( recv_buf.take( value) );
	fprintf( stderr, "ping tasklet %s in thread %s recevied %s\n", fss.str().c_str(), tss.str().c_str(), value->c_str());
	value.reset();

	send_buf.put("ping");
	BOOST_ASSERT( recv_buf.take( value) );
	fprintf( stderr, "ping tasklet %s in thread %s recevied %s\n", fss.str().c_str(), tss.str().c_str(), value->c_str());
	value.reset();

	send_buf.deactivate();

	fprintf( stderr, "end ping tasklet %s in thread %s\n", fss.str().c_str(), tss.str().c_str() );
}

inline
void pong(
		fifo_t & recv_buf,
		fifo_t & send_buf)
{
	std::stringstream tss;
	tss << boost::this_thread::get_id();
	std::stringstream fss;
	fss << boost::this_tasklet::get_id();

	fprintf( stderr, "start pong tasklet %s in thread %s\n", fss.str().c_str(), tss.str().c_str() );

	boost::optional< std::string > value;

	BOOST_ASSERT( recv_buf.take( value) );
	fprintf( stderr, "pong tasklet %s in thread %s recevied %s\n", fss.str().c_str(), tss.str().c_str(), value->c_str());
	value.reset();
	send_buf.put("pong");

	BOOST_ASSERT( recv_buf.take( value) );
	fprintf( stderr, "pong tasklet %s in thread %s recevied %s\n", fss.str().c_str(), tss.str().c_str(), value->c_str());
	value.reset();
	send_buf.put("pong");

	BOOST_ASSERT( recv_buf.take( value) );
	fprintf( stderr, "pong tasklet %s in thread %s recevied %s\n", fss.str().c_str(), tss.str().c_str(), value->c_str());
	value.reset();
	send_buf.put("pong");

	send_buf.deactivate();

	fprintf( stderr, "end pong tasklet %s in thread %s\n", fss.str().c_str(), tss.str().c_str() );
}

void f(
		fifo_t & recv_buf,
		fifo_t & send_buf)
{
	std::stringstream tss;
	tss << boost::this_thread::get_id();

	fprintf( stderr, "start thread %s\n", tss.str().c_str() );

	boost::tasklets::scheduler<> sched;
	
	sched.submit_tasklet(
		boost::tasklet(
			& ping, boost::ref( recv_buf), boost::ref( send_buf), boost::tasklet::default_stacksize, boost::protected_stack_allocator()) );

	for (;;)
	{
		while ( sched.run() );
		if ( sched.empty() ) break;
	}

	fprintf( stderr, "end thread %s\n", tss.str().c_str() );
}

void g(
		fifo_t & recv_buf,
		fifo_t & send_buf)
{
	std::stringstream tss;
	tss << boost::this_thread::get_id();

	fprintf( stderr, "start thread %s\n", tss.str().c_str() );

	boost::tasklets::scheduler<> sched;
	
	sched.submit_tasklet(
		boost::tasklet(
			& pong, boost::ref( recv_buf), boost::ref( send_buf), boost::tasklet::default_stacksize, boost::protected_stack_allocator()) );

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
		fifo_t buf1;
		fifo_t buf2;

		std::cout << "start" << std::endl;

		boost::thread th1( boost::bind( & f, boost::ref( buf1), boost::ref( buf2) ) );
		boost::thread th2( boost::bind( & g, boost::ref( buf2), boost::ref( buf1) ) );

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
