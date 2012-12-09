#include <cstdlib>
#include <iostream>
#include <string>

#include <boost/assert.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/ref.hpp>
#include <boost/optional.hpp>

#include <boost/fiber/all.hpp>

namespace stm = boost::fibers;
namespace this_stm = boost::this_fiber;

typedef stm::unbounded_channel< std::string >	fifo_t;

inline
void ping( fifo_t & recv_buf, fifo_t & send_buf)
{
    stm::fiber::id id( this_stm::get_id() );

	boost::optional< std::string > value;

	send_buf.put("ping");
	BOOST_ASSERT( recv_buf.take( value) );
	std::cout << "statum " <<  id << ": ping received: " << * value << std::endl;
	value.reset();

	send_buf.put("ping");
	BOOST_ASSERT( recv_buf.take( value) );
	std::cout << "statum " <<  id << ": ping received: " << * value << std::endl;
	value.reset();

	send_buf.put("ping");
	BOOST_ASSERT( recv_buf.take( value) );
	std::cout << "statum " <<  id << ": ping received: " << * value << std::endl;
	value.reset();

	send_buf.deactivate();
}

inline
void pong( fifo_t & recv_buf, fifo_t & send_buf)
{
    stm::fiber::id id( this_stm::get_id() );

	boost::optional< std::string > value;

	BOOST_ASSERT( recv_buf.take( value) );
	std::cout << "statum " <<  id << ": pong received: " << * value << std::endl;
	value.reset();
	send_buf.put("pong");

	BOOST_ASSERT( recv_buf.take( value) );
	std::cout << "statum " <<  id << ": pong received: " << * value << std::endl;
	value.reset();
	send_buf.put("pong");

	BOOST_ASSERT( recv_buf.take( value) );
	std::cout << "statum " <<  id << ": pong received: " << * value << std::endl;
	value.reset();
	send_buf.put("pong");

	send_buf.deactivate();
}

void f()
{
	fifo_t buf1, buf2;
	
	stm::fiber s1(
            boost::bind(
                & ping, boost::ref( buf1), boost::ref( buf2) ) );
	stm::fiber s2(
            boost::bind(
                & pong, boost::ref( buf2), boost::ref( buf1) ) );

    stm::waitfor_all( s1, s2);

    std::cout << "both channels deactivated" << std::endl;
}

int main()
{
	try
	{
		stm::fiber s( f);

        while ( s) stm::run();

		std::cout << "done." << std::endl;

		return EXIT_SUCCESS;
	}
	catch ( std::exception const& e)
	{ std::cerr << "exception: " << e.what() << std::endl; }
	catch (...)
	{ std::cerr << "unhandled exception" << std::endl; }
	return EXIT_FAILURE;
}
