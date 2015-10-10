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

#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>

#include <boost/fiber/all.hpp>

#include "round_robin.hpp"
#include "spawn.hpp"

void foo( boost::asio::io_service & io_svc, boost::fibers::asio::yield_context yield) {
    boost::asio::steady_timer timer( io_svc);
    boost::system::error_code ignored_ec;

    std::cout << "foo(): sleep 1s" << std::endl;
    boost::this_fiber::sleep_for( std::chrono::seconds( 1) );
    std::cout << "foo(): woken up after 1s" << std::endl;

    std::cout << "foo(): sleep 1s" << std::endl;
    timer.expires_from_now( std::chrono::seconds( 1) );
    timer.async_wait( boost::fibers::asio::yield[ignored_ec]);
    std::cout << "foo(): woken up after 1s" << std::endl;

    std::cout << "foo(): sleep 1s" << std::endl;
    timer.expires_from_now( std::chrono::seconds( 1) );
    timer.async_wait( boost::fibers::asio::yield[ignored_ec]);
    std::cout << "foo(): woken up after 1s" << std::endl;

    std::cout << "foo(): sleep 1s" << std::endl;
    boost::this_fiber::sleep_for( std::chrono::seconds( 1) );
    std::cout << "foo(): woken up after 1s" << std::endl;
}

int main( int argc, char* argv[]) {
    try {
        boost::asio::io_service io_svc;
        boost::fibers::use_scheduling_algorithm< boost::fibers::asio::round_robin >( io_svc);

        boost::fibers::asio::spawn( io_svc, std::bind( foo, std::ref( io_svc), std::placeholders::_1) );

        io_svc.run();

        return EXIT_SUCCESS;
    } catch ( std::exception const& e) {
        std::cerr << "Exception: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
}
