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
    try {
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
    } catch ( boost::fibers::fiber_interrupted const&) {
        std::cout << "fiber " << id << " : interrupted" << std::endl;
    } catch ( std::exception const& ex) {
        std::cout << "fiber " << id << " : catched exception : " << ex.what() << std::endl;
    }
    std::cout << "fiber " << id << " terminates" << std::endl;
}

void server( boost::asio::io_service & io_svc) {
    boost::fibers::fiber::id id = boost::this_fiber::get_id();
    std::cout << "fiber " << id << " : echo-server started" << std::endl;
    try {
        tcp::acceptor a( io_svc, tcp::endpoint( tcp::v4(), 9999) );
        for (;;) {
            socket_ptr socket( new tcp::socket( io_svc) );
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
    } catch ( boost::fibers::fiber_interrupted const&) {
        std::cout << "fiber " << id << " : interrupted" << std::endl;
    } catch ( std::exception const& ex) {
        std::cout << "fiber " << id << " : catched exception : " << ex.what() << std::endl;
    }
    std::cout << "fiber " << id << " terminates" << std::endl;
}

void client( boost::asio::io_service & io_svc) {
    tcp::resolver resolver( io_svc);
    tcp::resolver::query query( tcp::v4(), "127.0.0.1", "9999");
    tcp::resolver::iterator iterator = resolver.resolve( query);
    tcp::socket s( io_svc);
    boost::asio::connect( s, iterator);
    std::cout << "Enter message: ";
    char request[max_length];
    std::cin.getline( request, max_length);
    boost::system::error_code ec;
    size_t request_length = std::strlen( request);
            boost::asio::async_write(
                    s,
                    boost::asio::buffer( request, request_length),
                    boost::fibers::asio::yield[ec]);
            if ( ec == boost::asio::error::eof) {
                return; //connection closed cleanly by peer
            } else if ( ec) {
                throw boost::system::system_error( ec); //some other error
            }
    char reply[max_length];
    size_t reply_length = s.async_read_some(
            boost::asio::buffer( reply, request_length),
            boost::fibers::asio::yield[ec]);
    std::cout << "Reply is: ";
    std::cout.write( reply, reply_length);
}

int main( int argc, char* argv[]) {
    try {
        boost::fibers::fiber::id id = boost::this_fiber::get_id();
        std::cout << "fiber " << id << " : (main-fiber) started" << std::endl;
        boost::asio::io_service io_svc;
        boost::fibers::use_scheduling_algorithm< boost::fibers::asio::round_robin >( io_svc);
        // server fiber
        boost::fibers::fiber f(
            server, boost::ref( io_svc) );
        // client fiber
        boost::fibers::fiber(
            client, boost::ref( io_svc) ).detach();
        // fiber unrelated to asio
        boost::fibers::fiber(
            [](){
                boost::fibers::fiber::id id = boost::this_fiber::get_id();
                std::cout << "fiber " << id << " : sleeper started" << std::endl;
                for ( int i = 0; i < 5; ++i) {
                    std::cout << "fiber " << id << " : sleeps for 1 second" << std::endl;
                    boost::this_fiber::sleep_for( std::chrono::seconds( 1) );
                }
                std::cout << "fiber " << id << " : sleeps for 10 seconds" << std::endl;
                boost::this_fiber::sleep_for( std::chrono::seconds( 10) );
                for ( int i = 0; i < 5; ++i) {
                    std::cout << "fiber " << id << " : sleeps for 1 second" << std::endl;
                    boost::this_fiber::sleep_for( std::chrono::seconds( 1) );
                }
                std::cout << "fiber " << id << " sleeper terminates" << std::endl;
            }
        ).detach();
        // fiber does shutdown the io_service
        boost::fibers::fiber([&io_svc,&f]() mutable {
                        boost::fibers::fiber::id id = boost::this_fiber::get_id();
                        std::cout << "fiber " << id << " : shutdown io_service in 30 seconds" << std::endl;
                        boost::this_fiber::sleep_for( std::chrono::seconds( 30) );
                        std::cout << "fiber " << id << " : shutdown" << std::endl;
                        // stop io_service
                        io_svc.stop();
                        // interrupt
                        f.interrupt();
                        f.detach();
                        std::cout << "fiber " << id << " terminates" << std::endl;
                      }).detach();
        // run io_service
        io_svc.run();
        std::cout << "fiber " << id << " (main-fiber) terminates" << std::endl;
        std::cout << "done." << std::endl;
        return EXIT_SUCCESS;
    } catch ( std::exception const& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return EXIT_FAILURE;
}
