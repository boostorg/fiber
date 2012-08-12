
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// adapted version of echo-server example using coroutines from 
// Chris Kohlhoff: http://blog.think-async.com/2009/08/secret-sauce-revealed.html 

#include <cstdlib>
#include <iostream>

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/function.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/fiber/all.hpp>
#include <boost/system/error_code.hpp>
#include <boost/tuple/tuple.hpp>

class resolver_promise
{
private:
    typedef boost::tuple<
        const boost::system::error_code,
        boost::asio::ip::tcp::resolver::iterator
    >                   result_type;
    typedef boost::function<
        void(
            const boost::system::error_code,
            boost::asio::ip::tcp::resolver::iterator)
    >                   callback_type;

    void callback_impl(
        const boost::system::error_code ec,
        boost::asio::ip::tcp::resolver::iterator i)
    { prom_.set_value( result_type( ec, i) ); }

    boost::fibers::promise< result_type >   prom_;

public:
    resolver_promise() :
        prom_()
    {}

    boost::fibers::unique_future< result_type > future()
    { return prom_.get_future(); }

    callback_type callback()
    { return boost::bind( & resolver_promise::callback_impl, this, _1, _2); }
};

void handle_resolve(
    std::string const& hostname,
    boost::system::error_code const& err,
    boost::asio::ip::tcp::resolver::iterator i)
{
    if ( ! err)
    {
        boost::asio::ip::tcp::resolver::iterator e;
        for (; i != e; ++i)
        {
            boost::asio::ip::tcp::endpoint ep = * i;
            std::cout << hostname << " has address " << ep << std::endl;
        }
    }
    else
    {
        std::cout << "Error: " << err.message() << "\n";
    }
}


int main( int argc, char * argv[])
{
    try
    {
        if ( argc < 2)
        {
            std::cerr << "Usage: host <hostname> <service>\n";
            return 1;
        }
        {
            std::string hostname( argv[1]);
            std::string service( argc == 3 ? argv[2] : "");

            boost::asio::io_service io_service;
            boost::asio::ip::tcp::resolver resolver( io_service);

            boost::asio::ip::tcp::resolver::query query( hostname, service);
            resolver.async_resolve(
                query,
                boost::bind(
                    & handle_resolve,
                    hostname,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::iterator) );

            io_service.run();
        }
        std::cout << "Done" << std::endl;

        return  EXIT_SUCCESS;
    }
    catch ( std::exception const& e)
    { std::cerr << "Exception: " << e.what() << std::endl; }

    return EXIT_FAILURE;
}

