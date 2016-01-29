//
// echo_server.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//               2013      Oliver Kowalke
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>

#include <boost/fiber/all.hpp>
#include "round_robin.hpp"
#include "yield.hpp"

using boost::asio::ip::tcp;

const int max_length = 1024;

typedef boost::shared_ptr< tcp::socket > socket_ptr;

void session( socket_ptr sock) {
    boost::fibers::fiber::id id = boost::this_fiber::get_id();
    std::cout << "fiber " << id << " : echo-handler started" << std::endl;
    for (;;) {
        char data[max_length];
        boost::system::error_code ec;
        std::size_t length = sock->async_read_some(
                boost::asio::buffer( data),
                boost::fibers::asio::yield[ec]);
        if ( ec == boost::asio::error::eof) {
            break; //connection closed cleanly by peer
        } else if ( ec) {
            std::cerr << "fiber " << id << " : error occured : " << ec.message() << std::endl;
            return;
        }
        boost::asio::async_write(
                * sock,
                boost::asio::buffer( data, length),
                boost::fibers::asio::yield[ec]);
        if ( ec == boost::asio::error::eof) {
            break; //connection closed cleanly by peer
        } else if ( ec) {
            throw boost::system::system_error( ec); //some other error
        }
    }
}

void server( boost::asio::io_service & io_service, unsigned short port) {
    boost::fibers::fiber::id id = boost::this_fiber::get_id();
    std::cout << "fiber " << id << " : echo-server started" << std::endl;
    tcp::acceptor a( io_service, tcp::endpoint( tcp::v4(), port) );
    for (;;) {
        socket_ptr socket( new tcp::socket( io_service) );
        boost::system::error_code ec;
        std::cout << "fiber " << id << " : accept new connection" << std::endl;
        a.async_accept(
                * socket,
                boost::fibers::asio::yield[ec]);
        if ( ec) {
            std::cerr << "fiber " << id << " : error occured : " << ec.message() << std::endl;
        } else {
            boost::fibers::fiber( session, socket).detach();
        }
    }
}

int main( int argc, char* argv[]) {
    try {
        if ( 2 != argc) {
            std::cerr << "Usage: echo_server <port>\n";
            return EXIT_FAILURE;
        }
        boost::asio::io_service io_service;
        boost::fibers::use_scheduling_algorithm< boost::fibers::asio::round_robin >( io_service);
        // server fiber
        boost::fibers::fiber(
            server, boost::ref( io_service), std::atoi( argv[1]) ).detach();
        // fiber unrelated to asio
        boost::fibers::fiber(
            [](){
                boost::fibers::fiber::id id = boost::this_fiber::get_id();
                std::cout << "fiber " << id << " : sleeper tarted" << std::endl;
                for ( int i = 0; i < 1; ++i) {
                    std::cout << "fiber " << id << " : sleeps for 1 second" << std::endl;
                    boost::this_fiber::sleep_for( std::chrono::seconds( 1) );
                }
                std::cout << "fiber " << id << " : sleeps for 10 seconds" << std::endl;
                boost::this_fiber::sleep_for( std::chrono::seconds( 2) );
                for ( int i = 0; i < 1; ++i) {
                    std::cout << "fiber " << id << " : sleeps for 1 second" << std::endl;
                    boost::this_fiber::sleep_for( std::chrono::seconds( 1) );
                }
            }
        ).detach();
        // fiber does shutdown the io_service
        boost::fibers::fiber f([&io_service]() mutable {
                        boost::fibers::fiber::id id = boost::this_fiber::get_id();
                        std::cout << "fiber " << id << " : shutdown io_service in 10 seconds" << std::endl;
                        boost::this_fiber::sleep_for( std::chrono::seconds( 10) );
                        std::cout << "fiber " << id << " : shutdown" << std::endl;
                        io_service.stop();
                      });
        // run io_service
        io_service.run();
        // join fiber shutdown the io_service
        f.join();
        return EXIT_SUCCESS;
    } catch ( std::exception const& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return EXIT_FAILURE;
}
