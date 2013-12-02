#include <cstdlib>
#include <iostream>
#include <string>

#include <boost/assert.hpp>
#include <boost/bind.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/ref.hpp>
#include <boost/optional.hpp>

#include <boost/fiber/all.hpp>

typedef boost::fibers::unbounded_queue< std::string >	fifo_t;

inline
void ping( fifo_t & recv_buf, fifo_t & send_buf)
{
    boost::fibers::fiber::id id( boost::this_fiber::get_id() );

	send_buf.push("ping");

	std::string value;
    recv_buf.pop( value);
	std::cout << "fiber " <<  id << ": ping received: " << value << std::endl;
	value.clear();

	send_buf.push("ping");

    recv_buf.pop( value);
	std::cout << "fiber " <<  id << ": ping received: " << value << std::endl;
	value.clear();

	send_buf.push("ping");

    recv_buf.pop( value);
	std::cout << "fiber " <<  id << ": ping received: " << value << std::endl;

    send_buf.close();
}

inline
void pong( fifo_t & recv_buf, fifo_t & send_buf)
{
    boost::fibers::fiber::id id( boost::this_fiber::get_id() );

	std::string value;
    recv_buf.pop( value);
	std::cout << "fiber " <<  id << ": pong received: " << value << std::endl;
	value.clear();

	send_buf.push("pong");

    recv_buf.pop( value);
	std::cout << "fiber " <<  id << ": pong received: " << value << std::endl;
	value.clear();

	send_buf.push("pong");

    recv_buf.pop( value);
	std::cout << "fiber " <<  id << ": pong received: " << value << std::endl;

	send_buf.push("pong");

    send_buf.close();
}

int main()
{
	try
	{
        fifo_t buf1, buf2;

        boost::fibers::fiber f1(
                boost::bind(
                    & ping, boost::ref( buf1), boost::ref( buf2) ) );
        boost::fibers::fiber f2(
                boost::bind(
                    & pong, boost::ref( buf2), boost::ref( buf1) ) );

        f1.join();
        f2.join();

		std::cout << "done." << std::endl;

		return EXIT_SUCCESS;
	}
	catch ( std::exception const& e)
	{ std::cerr << "exception: " << e.what() << std::endl; }
	catch (...)
	{ std::cerr << "unhandled exception" << std::endl; }
	return EXIT_FAILURE;
}
