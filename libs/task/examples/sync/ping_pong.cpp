#include <cstdlib>
#include <iostream>
#include <string>
#include <sstream>

#include <boost/assert.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/ref.hpp>
#include <boost/optional.hpp>
#include <boost/system/system_error.hpp>
#include <boost/thread.hpp>

#include <boost/task.hpp>

typedef boost::tasks::static_pool< boost::tasks::unbounded_fifo >	pool_t;
typedef boost::tasks::spin::unbounded_channel< std::string >		fifo_t;

inline
void ping(
		fifo_t & recv_buf,
		fifo_t & send_buf,
		boost::tasks::spin::barrier & b)
{
	boost::this_thread::sleep( boost::posix_time::seconds( 1) );
	b.wait();

	std::stringstream tss;
	tss << boost::this_thread::get_id();
	std::stringstream fss;
	fss << boost::this_tasklet::get_id();

	fprintf( stderr, "start tasklet task %s in thread %s\n", fss.str().c_str(), tss.str().c_str() );

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
		fifo_t & send_buf,
		boost::tasks::spin::barrier & b)
{
	b.wait();
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

int main()
{
	try
	{
		fifo_t buf1;
		fifo_t buf2;
		boost::tasks::spin::barrier b( 2);

		std::cout << "start" << std::endl;

		pool_t pool( boost::tasks::poolsize( 2) );

		boost::tasks::async(
			boost::tasks::make_task( ping, buf1, buf2, boost::ref( b) ),
			pool);

		boost::tasks::async(
			boost::tasks::make_task( pong, buf2, buf1, boost::ref( b) ),
			pool);

		pool.shutdown();

		std::cout << "finish" << std::endl;

		return EXIT_SUCCESS;
	}
	catch ( boost::system::system_error const& e)
	{ std::cerr << "system_error: " << e.code().value() << std::endl; }
	catch ( std::exception const& e)
	{ std::cerr << "exception: " << e.what() << std::endl; }
	catch (...)
	{ std::cerr << "unhandled exception" << std::endl; }
	return EXIT_FAILURE;
}
