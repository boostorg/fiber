//
// daytime_client.cpp
// ~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <iostream>

#include <boost/array.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/bind.hpp>
#include <boost/ref.hpp>
#include <boost/system/system_error.hpp>

#include <boost/fiber/all.hpp>

#include "use_ffuture.hpp"

using boost::asio::ip::udp;

void get_daytime(boost::asio::io_service& io_service, const char* hostname)
{
  try
  {
    udp::resolver resolver(io_service);

    boost::fibers::future<udp::resolver::iterator> iter =
      resolver.async_resolve(
          udp::resolver::query( udp::v4(), hostname, "daytime"),
          boost::asio::use_ffuture);

    // The async_resolve operation above returns the endpoint iterator as a
    // future value that is not retrieved ...

    udp::socket socket(io_service, udp::v4());

    boost::array<char, 1> send_buf  = {{ 0 }};
    boost::fibers::future<std::size_t> send_length =
      socket.async_send_to(boost::asio::buffer(send_buf),
          *iter.get(), // ... until here. This call may block.
          boost::asio::use_ffuture);

    // Do other things here while the send completes.

    send_length.get(); // Blocks until the send is complete. Throws any errors.

    boost::array<char, 128> recv_buf;
    udp::endpoint sender_endpoint;
    boost::fibers::future<std::size_t> recv_length =
      socket.async_receive_from(
          boost::asio::buffer(recv_buf),
          sender_endpoint,
          boost::asio::use_ffuture);

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

int main(int argc, char* argv[])
{
    boost::fibers::round_robin ds;
    boost::fibers::scheduling_algorithm( & ds);
  try
  {
    if (argc != 2)
    {
      std::cerr << "Usage: daytime_client <host>" << std::endl;
      return 1;
    }

    // We run the io_service off in its own thread so that it operates
    // completely asynchronously with respect to the rest of the program.
    boost::asio::io_service io_service;
    boost::asio::io_service::work work(io_service);

#if 0
    //boost::thread tsk([&io_service](){ io_service.run(); });
    boost::fibers::fiber tsk([&io_service](){ io_service.run(); }); // blocks main-thread
    get_daytime(io_service, argv[1]);
    io_service.stop();
    tsk.join();
#endif

    //std::thread tsk( boost::bind( get_daytime, boost::ref( io_service), argv[1]) );
    boost::fibers::fiber tsk( boost::bind( get_daytime, boost::ref( io_service), argv[1]) );
    io_service.run();
    tsk.join();
  }
  catch (std::exception& e)
  {
    std::cerr << e.what() << std::endl;
  }

  return 0;
}
