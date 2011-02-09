#include <cstdlib>
#include <iostream>
#include <string>

#include <boost/assert.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/ref.hpp>
#include <boost/optional.hpp>

#include <boost/tasklet.hpp>

typedef boost::tasklets::unbounded_channel< std::string >	fifo_t;
typedef boost::intrusive_ptr< fifo_t >			fifo_ptr;

inline
void ping(
		fifo_t & recv_buf,
		fifo_t & send_buf)
{
	boost::optional< std::string > value;

	send_buf.put("ping");
	BOOST_ASSERT( recv_buf.take( value) );
	std::cout << * value << std::endl;
	value.reset();

	send_buf.put("ping");
	BOOST_ASSERT( recv_buf.take( value) );
	std::cout << * value << std::endl;
	value.reset();

	send_buf.put("ping");
	BOOST_ASSERT( recv_buf.take( value) );
	std::cout << * value << std::endl;
	value.reset();

	send_buf.deactivate();
}

inline
void pong(
		fifo_t & recv_buf,
		fifo_t & send_buf)
{
	boost::optional< std::string > value;

	BOOST_ASSERT( recv_buf.take( value) );
	std::cout << * value << std::endl;
	value.reset();
	send_buf.put("pong");

	BOOST_ASSERT( recv_buf.take( value) );
	std::cout << * value << std::endl;
	value.reset();
	send_buf.put("pong");

	BOOST_ASSERT( recv_buf.take( value) );
	std::cout << * value << std::endl;
	value.reset();
	send_buf.put("pong");

	send_buf.deactivate();
}

void f( boost::tasklets::scheduler<> & sched)
{
	fifo_t buf1;
	fifo_t buf2;
	
	sched.submit_tasklet(
		boost::tasklet(
			& ping, boost::ref( buf1), boost::ref( buf2), boost::tasklet::default_stacksize, boost::protected_stack_allocator()) );
	sched.submit_tasklet(
		boost::tasklet(
			& pong, boost::ref( buf2), boost::ref( buf1), boost::tasklet::default_stacksize, boost::protected_stack_allocator()) );
}

int main()
{
	try
	{
		boost::tasklets::scheduler<> sched;

		sched.submit_tasklet(
			boost::tasklet(
				& f, boost::ref( sched), boost::tasklet::default_stacksize, boost::protected_stack_allocator()) );

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
