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

#include <boost/fiber/all.hpp>

#include "asio_scheduler.hpp"
#include "spawn.hpp"

void foo( boost::fibers::asio::yield_context yield) {
    std::cout << "sleep 1s" << std::endl;
    boost::this_fiber::sleep_for( std::chrono::seconds( 1) );
    std::cout << "woken up after 1s" << std::endl;
}

int main( int argc, char* argv[]) {
    try {
        boost::asio::io_service io_svc;
        boost::fibers::use_scheduling_algorithm< asio_scheduler >( io_svc);

        boost::fibers::asio::spawn( io_svc, std::bind( foo, std::placeholders::_1) );

        io_svc.run();

        return EXIT_SUCCESS;
    } catch ( std::exception const& e) {
        std::cerr << "Exception: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
}
