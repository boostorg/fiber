
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
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/fiber/all.hpp>
#include <boost/system/error_code.hpp>
#include <boost/tuple/tuple.hpp>

class server : public boost::enable_shared_from_this< server >
{
private:
    typedef boost::coro::coroutine< void( boost::system::error_code, std::size_t) >  coro_t;
    typedef boost::tuple< boost::system::error_code, std::size_t >                  tuple_t;

    boost::asio::ip::tcp::acceptor      acceptor_;
    coro_t                              coro_;

    void do_( coro_t::self_t & self, boost::system::error_code ec, std::size_t n)
    {
       for (;;)
       {
            boost::asio::ip::tcp::socket socket( acceptor_.get_io_service() );
            acceptor_.async_accept( socket, boost::bind( & server::operator(), this->shared_from_this(), _1, 0) );
            tuple_t tpl = self.yield();

            while ( ! tpl.get< 0 >() )
            {
                boost::array< char, 1024 > buffer;

                socket.async_read_some(
                    boost::asio::buffer( buffer),
                    boost::bind( & server::operator(), this->shared_from_this(), _1, _2) ); 
                tpl = self.yield();

                if ( tpl.get< 0 >() ) break;

                boost::asio::async_write(
                    socket,
                    boost::asio::buffer( buffer, tpl.get< 1 >() ),
                    boost::bind( & server::operator(), this->shared_from_this(), _1, _2) ); 
                tpl = self.yield();
            }
       }
    }

    server( boost::asio::io_service & io_service, short port) :
        acceptor_( io_service, boost::asio::ip::tcp::endpoint( boost::asio::ip::tcp::v4(), port) ),
        coro_( boost::bind( & server::do_, this, _1, _2, _3) )
    {}

public:
    typedef boost::shared_ptr< server >     ptr_t;

    static ptr_t create( boost::asio::io_service & io_service, short port)
    { return ptr_t( new server( io_service, port) ); }

    void operator()( boost::system::error_code ec, size_t n)
    {
        coro_( ec, n);
    }
};

int main( int argc, char * argv[])
{
    try
    {
        if ( argc != 2)
        {
            std::cerr << "Usage: echo_server <port>\n";
            return 1;
        }
        {
            boost::asio::io_service io_service;
            io_service.post(
                boost::bind(
                    & server::operator(),
                    server::create( io_service, boost::lexical_cast< short >( argv[1]) ),
                    boost::system::error_code(), 0) );
            io_service.run();
        }
        std::cout << "Done" << std::endl;

        return  EXIT_SUCCESS;
    }
    catch ( std::exception const& e)
    { std::cerr << "Exception: " << e.what() << std::endl; }

    return EXIT_FAILURE;
}

