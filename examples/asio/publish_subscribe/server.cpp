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
#include <map>
#include <iostream>
#include <string>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/foreach.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>

#include <boost/fiber/all.hpp>

using boost::asio::ip::tcp;

const int max_length = 1024;

class subscriber_session;

class subscriptions
{
public:
    ~subscriptions();

    void subscribe( boost::shared_ptr< subscriber_session > const& s)
    { subscribers_.push_back( s); }

    void unsubscribe( boost::shared_ptr< subscriber_session > const& s)
    {
        std::vector< boost::shared_ptr< subscriber_session > >::iterator e = subscribers_.end();
        for (
            std::vector< boost::shared_ptr< subscriber_session > >::iterator i = subscribers_.begin();
            i != e;
            ++i)
        {
            if ( * i == s)
            {
                subscribers_.erase( i);
                break;
            }
        }
    }

    void publish( std::string const& msg);

private:
    std::vector< boost::shared_ptr< subscriber_session > >  subscribers_;
};


class registry : private boost::noncopyable
{
private:
    boost::fibers::mutex                                            mtx_;
    std::map< std::string, boost::shared_ptr< subscriptions > >     channels_;

    void register_channel_( std::string const& channel)
    {
        if ( channels_.end() != channels_.find( channel) )
            throw std::runtime_error("channel already exists");
        channels_[channel] = boost::shared_ptr< subscriptions >(
                new subscriptions() );
        std::cout << "new channel '" << channel << "' registered" << std::endl;
    }

    void unregister_channel_( std::string const& channel)
    {
        channels_.erase( channel);
        std::cout << "channel '" << channel << "' unregistered" << std::endl;
    }

    void subscribe_( std::string const& channel, boost::shared_ptr< subscriber_session > s)
    {
        if ( channels_.end() == channels_.find( channel) )
            throw std::runtime_error("channel does not exist");
        channels_.find( channel)->second->subscribe( s);
        std::cout << "new subscribtion to channel '" << channel << "'" << std::endl;
    }

    void unsubscribe_( std::string const& channel, boost::shared_ptr< subscriber_session > s)
    {
        if ( channels_.end() != channels_.find( channel) )
            channels_.find( channel)->second->unsubscribe( s);
    }

    void publish_( std::string const& channel, std::string const& msg)
    {
        BOOST_ASSERT( channels_.end() != channels_.find( channel) );
        channels_.find( channel)->second->publish( msg);
        std::cout << "message '" << msg << "' to publish on channel '" << channel << "'" << std::endl;
    }

public:
    void register_channel( std::string const& channel)
    {
        boost::fibers::mutex::scoped_lock lk( mtx_);
        register_channel_( channel);
    }

    void unregister_channel( std::string const& channel)
    {
        boost::fibers::mutex::scoped_lock lk( mtx_);
        unregister_channel_( channel);
    }

    void subscribe( std::string const& channel, boost::shared_ptr< subscriber_session > s)
    {
        boost::fibers::mutex::scoped_lock lk( mtx_);
        subscribe_( channel, s);
    }

    void unsubscribe( std::string const& channel, boost::shared_ptr< subscriber_session > s)
    {
        boost::fibers::mutex::scoped_lock lk( mtx_);
        unsubscribe_( channel, s);
    }


    void publish( std::string const& channel, std::string const& msg)
    {
        boost::fibers::mutex::scoped_lock lk( mtx_);
        publish_( channel, msg);
    }
};


class subscriber_session : public boost::enable_shared_from_this< subscriber_session >
{
public:
    explicit subscriber_session( boost::asio::io_service & io_service, registry & reg) :
        socket_( io_service),
        reg_( reg)
    {}

    tcp::socket& socket()
    { return socket_; }

    void run( boost::fibers::asio::yield_context yield)
    {
        std::string channel;
        try
        {
            // read first message == channel name
            boost::system::error_code ec;

            boost::asio::async_read(
                    socket_,
                    boost::asio::buffer( data_),
                    yield[ec]);
            if ( ec) throw std::runtime_error("no channel from subscriber");
            channel = data_;
            
            // register new channel
            reg_.subscribe( channel, shared_from_this() );

            // wait for published messages
            for (;;)
            {
                boost::fibers::mutex::scoped_lock lk( mtx_);
                cond_.wait( lk);
                if ( "<fini>" == std::string( data_) ) break;

                boost::asio::async_write(
                        socket_,
                        boost::asio::buffer( data_, max_length),
                        yield[ec]);
                if ( ec) throw std::runtime_error("publishing message failed");
            }
        }
        catch ( std::exception const& e)
        { std::cerr << "subscriber [" << channel << "] failed: " << e.what() << std::endl; }

        // close socket
        socket_.close();
        // unregister channel
        reg_.unsubscribe( channel, shared_from_this() );
    }

    // called from publisher_session (running in other fiber)
    void publish( std::string const& msg)
    {
        boost::fibers::mutex::scoped_lock lk( mtx_);
        std::memset(data_, '\0', sizeof( data_));
        std::memcpy(data_, msg.c_str(), msg.size());
        cond_.notify_one();
    }

private:
    tcp::socket                         socket_;
    registry                        &   reg_;
    boost::fibers::mutex                mtx_;
    boost::fibers::condition_variable   cond_;
    // fixed size message
    char                                data_[max_length];
};


subscriptions::~subscriptions()
{
    BOOST_FOREACH( boost::shared_ptr< subscriber_session > s, subscribers_)
    { s->publish("<fini>"); } 
}

void
subscriptions::publish( std::string const& msg)
{
    BOOST_FOREACH( boost::shared_ptr< subscriber_session > s, subscribers_)
    { s->publish( msg); }
}


class publisher_session : public boost::enable_shared_from_this< publisher_session >
{
public:
    explicit publisher_session( boost::asio::io_service & io_service, registry & reg) :
        socket_( io_service),
        reg_( reg)
    {}

    tcp::socket& socket()
    { return socket_; }

    void run( boost::fibers::asio::yield_context yield)
    {
        std::string channel;
        try
        {
            // fixed size message
            char data[max_length];

            // read first message == channel name
            boost::system::error_code ec;
            boost::asio::async_read(
                    socket_,
                    boost::asio::buffer( data),
                    yield[ec]);
            if ( ec) throw std::runtime_error("no channel from publisher");
            channel = data;
            
            // register new channel
            reg_.register_channel( channel);

            // start publishing messages
            for (;;)
            {
                // read message from publisher
                boost::asio::async_read(
                        socket_,
                        boost::asio::buffer( data),
                        yield[ec]); // does not work -> scheduler::run() not called
                        //boost::fibers::asio::yield); 
                if ( ec == boost::asio::error::eof)
                    break; //connection closed cleanlyby peer
                else if ( ec)
                    throw boost::system::system_error( ec); //some other error

                // publish message to all subscribers
                reg_.publish( channel, std::string( data) );
            }
        }
        catch ( std::exception const& e)
        { std::cerr << "publisher [" << channel << "] failed: " << e.what() << std::endl; }

        // close socket
        socket_.close();
        // unregister channel
        reg_.unregister_channel( channel);
    }

private:
    tcp::socket         socket_;
    registry        &   reg_;
};


void accept_publisher( boost::asio::io_service& io_service,
                        unsigned short port,
                        registry & reg,
                        boost::fibers::asio::yield_context yield)
{
    tcp::acceptor acceptor( io_service, tcp::endpoint( tcp::v4(), port) );

    for (;;)
    {
        boost::system::error_code ec;
        boost::shared_ptr< publisher_session > new_publisher_session( new publisher_session( io_service, reg) );
        acceptor.async_accept(
                new_publisher_session->socket(), 
                yield[ec]);
        if ( ! ec) {
            boost::fibers::asio::spawn( io_service,
                boost::bind( & publisher_session::run, new_publisher_session, _1) );
        }
    }
}


void accept_subscriber( boost::asio::io_service& io_service,
                        unsigned short port,
                        registry & reg,
                        boost::fibers::asio::yield_context yield)
{
    tcp::acceptor acceptor( io_service, tcp::endpoint( tcp::v4(), port) );

    for (;;)
    {
        boost::system::error_code ec;
        boost::shared_ptr< subscriber_session > new_subscriber_session( new subscriber_session( io_service, reg) );
        acceptor.async_accept(
                new_subscriber_session->socket(), 
                yield[ec]);
        if ( ! ec) {
            boost::fibers::asio::spawn( io_service,
                boost::bind( & subscriber_session::run, new_subscriber_session, _1) );
        }
    }
}


int main( int argc, char* argv[])
{
    try
    {
        boost::asio::io_service io_service;
        boost::fibers::asio::round_robin ds( io_service);
        boost::fibers::set_scheduling_algorithm( & ds);

        registry reg;

        boost::fibers::asio::spawn( io_service,
            boost::bind( accept_publisher,
                boost::ref( io_service), 9997, boost::ref( reg), _1) );

        boost::fibers::asio::spawn( io_service,
            boost::bind( accept_subscriber,
                boost::ref( io_service), 9998, boost::ref( reg), _1) );

        io_service.run();
    }
    catch ( std::exception const& e)
    { std::cerr << "Exception: " << e.what() << "\n"; }

    return 0;
}
