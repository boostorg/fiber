//          Copyright 2003-2013 Christopher M. Kohlhoff
//          Copyright Oliver Kowalke, Nat Goodspeed 2015.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <sstream>
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
                throw boost::system::system_error( ec); //some other error
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
        std::ostringstream buffer;
        buffer << "tid=" << std::this_thread::get_id() << ", fid=" << id << " : interrupted" << std::endl;
        std::cerr << buffer.str() << std::flush;
    } catch ( std::exception const& ex) {
        std::ostringstream buffer;
        buffer << "tid=" << std::this_thread::get_id() << ", fid=" << id << " : catched exception : " << ex.what() << std::endl;
        std::cerr << buffer.str() << std::flush;
    }
}

void server( boost::asio::io_service & io_svc) {
    boost::fibers::fiber::id id = boost::this_fiber::get_id();
    std::ostringstream buffer;
    buffer << "tid=" << std::this_thread::get_id() << ", fid=" << id << " : echo-server started" << std::endl;
    std::cout << buffer.str() << std::flush;
    try {
        tcp::acceptor a( io_svc, tcp::endpoint( tcp::v4(), 9999) );
        for (;;) {
            socket_ptr socket( new tcp::socket( io_svc) );
            boost::system::error_code ec;
            a.async_accept(
                    * socket,
                    boost::fibers::asio::yield[ec]);
            if ( ec) {
                throw boost::system::system_error( ec); //some other error
            } else {
                boost::fibers::fiber( session, socket).detach();
            }
        }
    } catch ( boost::fibers::fiber_interrupted const&) {
        std::ostringstream buffer;
        buffer << "tid=" << std::this_thread::get_id() << ", fid=" << id << " : interrupted" << std::endl;
        std::cerr << buffer.str() << std::flush;
    } catch ( std::exception const& ex) {
        std::ostringstream buffer;
        buffer << "tid=" << std::this_thread::get_id() << ", fid=" << id << " : catched exception : " << ex.what() << std::endl;
        std::cerr << buffer.str() << std::flush;
    }
}

void client( boost::asio::io_service & io_svc) {
    boost::fibers::fiber::id id = boost::this_fiber::get_id();
    std::ostringstream buffer;
    buffer << "tid=" << std::this_thread::get_id() << ", fid=" << id << " : echo-client started" << std::endl;
    std::cout << buffer.str() << std::flush;
    tcp::resolver resolver( io_svc);
    tcp::resolver::query query( tcp::v4(), "127.0.0.1", "9999");
    tcp::resolver::iterator iterator = resolver.resolve( query);
    tcp::socket s( io_svc);
    boost::asio::connect( s, iterator);
    for (;;) {
        char request[max_length];
        std::ostringstream buffer;
        buffer << "tid=" << std::this_thread::get_id() << ", fid=" << id << " : Enter message: ";
        std::cout << buffer.str() << std::flush;
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
        if ( ec == boost::asio::error::eof) {
            return; //connection closed cleanly by peer
        } else if ( ec) {
            throw boost::system::system_error( ec); //some other error
        }
        std::ostringstream result;
        result << "tid=" << std::this_thread::get_id() << ", fid=" << id << " : Reply is: ";
        result.write( reply, reply_length);
        result << std::endl;
        std::cout << result.str() << std::flush;
    }
}

int main( int argc, char* argv[]) {
    try {
        boost::asio::io_service io_svc;
        boost::fibers::use_scheduling_algorithm< boost::fibers::asio::round_robin >( io_svc);
        // server
        boost::fibers::fiber f(
            server, boost::ref( io_svc) );
        // client
        boost::fibers::fiber(
            client, boost::ref( io_svc) ).detach();
        // run io_service in two threads
        std::thread t([&io_svc](){
                    boost::fibers::use_scheduling_algorithm< boost::fibers::asio::round_robin >( io_svc);
                    boost::fibers::asio::run_svc( io_svc);
                });
        boost::fibers::asio::run_svc( io_svc);
        t.join();
        std::cout << "done." << std::endl;
        return EXIT_SUCCESS;
    } catch ( std::exception const& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return EXIT_FAILURE;
}
