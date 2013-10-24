//
// daytime_client.cpp
// ~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// modified by Oliver Kowalke

#include <iostream>

#include <boost/array.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/bind.hpp>
#include <boost/ref.hpp>
#include <boost/system/system_error.hpp>

#include <boost/fiber/all.hpp>

using boost::asio::ip::udp;

void get_daytime(boost::asio::io_service& io_service, const char* hostname)
{
  try
  {
    udp::resolver resolver(io_service);

    boost::fibers::future<udp::resolver::iterator> iter =
      resolver.async_resolve(
          udp::resolver::query( udp::v4(), hostname, "daytime"),
          boost::fibers::asio::use_future);

    // The async_resolve operation above returns the endpoint iterator as a
    // future value that is not retrieved ...

    udp::socket socket(io_service, udp::v4());

    boost::array<char, 1> send_buf  = {{ 0 }};
    boost::fibers::future<std::size_t> send_length =
      socket.async_send_to(boost::asio::buffer(send_buf),
          *iter.get(), // ... until here. This call may block.
          boost::fibers::asio::use_future);

    // Do other things here while the send completes.

    send_length.get(); // Blocks until the send is complete. Throws any errors.

    boost::array<char, 128> recv_buf;
    udp::endpoint sender_endpoint;
    boost::fibers::future<std::size_t> recv_length =
      socket.async_receive_from(
          boost::asio::buffer(recv_buf),
          sender_endpoint,
          boost::fibers::asio::use_future);

    // Do other things here while the receive completes.

    std::cout.write(
        recv_buf.data(),
        recv_length.get()); // Blocks until receive is complete.
  }
  catch (boost::system::system_error& e)
  {
    std::cerr << e.what() << std::endl;
  }
  io_service.stop();
}

int main( int argc, char* argv[])
{
    boost::asio::io_service io_service;
    boost::fibers::asio::round_robin ds(io_service);
    boost::fibers::set_scheduling_algorithm( & ds);
    try
    {
        if (argc != 2)
        {
            std::cerr << "Usage: daytime_client <host>" << std::endl;
            return 1;
        }

        boost::fibers::fiber fiber( boost::bind( get_daytime, boost::ref( io_service), argv[1]) );
        io_service.run();
        fiber.join();
    }
    catch ( std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
