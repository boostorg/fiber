//
// echo_server2.cpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//               2013      Oliver Kowalke
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//

#include <cstdlib>
#include <iostream>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>

#include <boost/fiber/all.hpp>

using boost::asio::ip::tcp;

const int max_length = 1024;

class session : public boost::enable_shared_from_this< session >
{
public:
    explicit session( boost::asio::io_service & io_service) :
        strand_( io_service),
        socket_( io_service),
        timer_( io_service)
    {}

    tcp::socket& socket()
    { return socket_; }

    void go()
    {
        boost::fibers::fiber(
            boost::bind( & session::timeout, shared_from_this() ) ).detach();
        echo();
    }

private:
    void echo()
    {
        try
        {
            char data[max_length];
            for (;;)
            {
                timer_.expires_from_now(
                    boost::posix_time::seconds( 3) );
                std::size_t n = socket_.async_read_some(
                    boost::asio::buffer( data),
                    boost::fibers::asio::yield);
                boost::asio::async_write(
                    socket_,
                    boost::asio::buffer( data, n),
                    boost::fibers::asio::yield);
            }
        }
        catch ( std::exception const& e)
        {
            socket_.close();
            timer_.cancel();
        }
    }

    void timeout()
    {
        while ( socket_.is_open() )
        {
            boost::system::error_code ignored_ec;
            timer_.async_wait( boost::fibers::asio::yield[ignored_ec]);
            if ( timer_.expires_from_now() <= boost::posix_time::seconds( 0) ) {
                std::cout << "session to " << socket_.remote_endpoint() << " timed out" << std::endl;
                socket_.close();
            }
        }
    }

    boost::asio::io_service::strand   strand_;
    tcp::socket                       socket_;
    boost::asio::deadline_timer       timer_;
};

void do_accept( boost::asio::io_service & io_service, unsigned short port)
{
    tcp::acceptor acceptor( io_service, tcp::endpoint( tcp::v4(), port) );

    for (;;)
    {
        boost::system::error_code ec;
        boost::shared_ptr< session > new_session( new session( io_service) );
        acceptor.async_accept(
                new_session->socket(), 
                boost::fibers::asio::yield[ec]);
        if ( ! ec) {
            boost::fibers::fiber( boost::bind( & session::go, new_session) ).detach();
        }
    }
}

int main( int argc, char* argv[])
{
    try
    {
        if ( argc != 2)
        {
            std::cerr << "Usage: echo_server <port>\n";
            return 1;
        }

        boost::asio::io_service io_service;
        boost::fibers::asio::io_service ds( io_service);
        boost::fibers::scheduling_algorithm( & ds);

        using namespace std; // For atoi.
        boost::fibers::fiber fiber(
            boost::bind( do_accept, boost::ref( io_service), atoi( argv[1]) ) );
        io_service.run();
        fiber.join();
    }
    catch ( std::exception const& e)
    { std::cerr << "Exception: " << e.what() << "\n"; }

    return 0;
}
