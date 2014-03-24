//
// server.cpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2013 Oliver Kowalke
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

#include "../loop.hpp"
#include "../spawn.hpp"
#include "../yield.hpp"

using boost::asio::ip::tcp;

const int max_length = 1024;

class subscriber_session;

// a channel has n subscribers (subscriptions)
// this class holds a list of subcribers for one channel
class subscriptions
{
public:
    ~subscriptions();

    // subscribe to this channel
    void subscribe( boost::shared_ptr< subscriber_session > const& s)
    { subscribers_.push_back( s); }

    // unsubscribe from this channel
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

    // publish a message, e.g. push this message to all subscribers
    void publish( std::string const& msg);

private:
    // list of subscribers
    std::vector< boost::shared_ptr< subscriber_session > >  subscribers_;
};

// a class to register channels and to subsribe clients to this channels
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
        std::cout << "new subscription to channel '" << channel << "'" << std::endl;
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
    // add a channel to registry
    void register_channel( std::string const& channel)
    {
        boost::fibers::mutex::scoped_lock lk( mtx_);
        register_channel_( channel);
    }

    // remove a channel from registry
    void unregister_channel( std::string const& channel)
    {
        boost::fibers::mutex::scoped_lock lk( mtx_);
        unregister_channel_( channel);
    }

    // subscribe to a channel
    void subscribe( std::string const& channel, boost::shared_ptr< subscriber_session > s)
    {
        boost::fibers::mutex::scoped_lock lk( mtx_);
        subscribe_( channel, s);
    }

    // unsubscribe from a channel
    void unsubscribe( std::string const& channel, boost::shared_ptr< subscriber_session > s)
    {
        boost::fibers::mutex::scoped_lock lk( mtx_);
        unsubscribe_( channel, s);
    }

    // publish a message to all subscribers registerd to the channel
    void publish( std::string const& channel, std::string const& msg)
    {
        boost::fibers::mutex::scoped_lock lk( mtx_);
        publish_( channel, msg);
    }
};

// a subscriber subscribes to a given channel in order to receive messages published on this channel
class subscriber_session : public boost::enable_shared_from_this< subscriber_session >
{
public:
    explicit subscriber_session( boost::asio::io_service & io_service, registry & reg) :
        socket_( io_service),
        reg_( reg)
    {}

    tcp::socket& socket()
    { return socket_; }

    // this function is executed inside the fiber
    void run( boost::fibers::asio::yield_context yield)
    {
        std::string channel;
        try
        {
            boost::system::error_code ec;

            // read first message == channel name
            // async_ready() returns if the the complete message is read
            // until this the fiber is suspended until the complete message
            // is read int the given buffer 'data'
            boost::asio::async_read(
                    socket_,
                    boost::asio::buffer( data_),
                    yield[ec]);
            if ( ec) throw std::runtime_error("no channel from subscriber");
            // first message ist equal to the channel name the publisher
            // publishes to
            channel = data_;
            
            // subscribe to new channel
            reg_.subscribe( channel, shared_from_this() );

            // read published messages
            for (;;)
            {
                // wait for a conditon-variable for new messages
                // the fiber will be suspended until the condtion
                // becomes true and the fiber is resumed
                // published message is stored in buffer 'data_'
                boost::fibers::mutex::scoped_lock lk( mtx_);
                cond_.wait( lk);
                
                // message '<fini>' terminates subscription
                if ( "<fini>" == std::string( data_) ) break;

                // async. write message to socket connected with
                // subscriber
                // async_write() returns if the complete message was writen
                // the fiber is suspended in the meanwhile
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

// a publisher publishes messages on its channel
// subscriber might register to this channel to get the published messages
class publisher_session : public boost::enable_shared_from_this< publisher_session >
{
public:
    explicit publisher_session( boost::asio::io_service & io_service, registry & reg) :
        socket_( io_service),
        reg_( reg)
    {}

    tcp::socket& socket()
    { return socket_; }

    // this function is executed inside the fiber
    void run( boost::fibers::asio::yield_context yield)
    {
        std::string channel;
        try
        {
            boost::system::error_code ec;
            
            // fixed size message
            char data[max_length];

            // read first message == channel name
            // async_ready() returns if the the complete message is read
            // until this the fiber is suspended until the complete message
            // is read int the given buffer 'data'
            boost::asio::async_read(
                    socket_,
                    boost::asio::buffer( data),
                    yield[ec]);
            if ( ec) throw std::runtime_error("no channel from publisher");
            // first message ist equal to the channel name the publisher
            // publishes to
            channel = data;
            
            // register the new channel
            reg_.register_channel( channel);

            // start publishing messages
            for (;;)
            {
                // read message from publisher asyncronous
                // async_read() suspends this fiber until the complete emssage is read
                // and stored in the given buffer 'data'
                boost::asio::async_read(
                        socket_,
                        boost::asio::buffer( data),
                        yield[ec]); 
                if ( ec == boost::asio::error::eof)
                    break; //connection closed cleanly by peer
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

// function accepts connections requests from clients acting as a publisher
void accept_publisher( boost::asio::io_service& io_service,
                        unsigned short port,
                        registry & reg,
                        boost::fibers::asio::yield_context yield)
{
    // create TCP-acceptor
    tcp::acceptor acceptor( io_service, tcp::endpoint( tcp::v4(), port) );

    // loop for accepting connection requests
    for (;;)
    {
        boost::system::error_code ec;
        // create new publisher-session
        // this instance will be associated with one publisher
        boost::shared_ptr< publisher_session > new_publisher_session( new publisher_session( io_service, reg) );
        // async. accept of new connection request
        // this function will suspend this execution context (fiber) until a
        // connection was established, after returning from this function a new client (publisher)
        // is connected
        acceptor.async_accept(
                new_publisher_session->socket(), 
                yield[ec]);
        if ( ! ec) {
            // run the new publisher in its own fiber (one fiber for one client)
            boost::fibers::asio::spawn( io_service,
                boost::bind( & publisher_session::run, new_publisher_session, _1) );
        }
    }
}

// function accepts connections requests from clients acting as a subscriber
void accept_subscriber( boost::asio::io_service& io_service,
                        unsigned short port,
                        registry & reg,
                        boost::fibers::asio::yield_context yield)
{
    // create TCP-acceptor
    tcp::acceptor acceptor( io_service, tcp::endpoint( tcp::v4(), port) );

    // loop for accepting connection requests
    for (;;)
    {
        boost::system::error_code ec;
        // create new subscriber-session
        // this instance will be associated with one subscriber
        boost::shared_ptr< subscriber_session > new_subscriber_session( new subscriber_session( io_service, reg) );
        // async. accept of new connection request
        // this function will suspend this execution context (fiber) until a
        // connection was established, after returning from this function a new client (subscriber)
        // is connected
        acceptor.async_accept(
                new_subscriber_session->socket(), 
                yield[ec]);
        if ( ! ec) {
            // run the new subscriber in its own fiber (one fiber for one client)
            boost::fibers::asio::spawn( io_service,
                boost::bind( & subscriber_session::run, new_subscriber_session, _1) );
        }
    }
}


int main( int argc, char* argv[])
{
    try
    {
        // create io_service for async. I/O
        boost::asio::io_service io_service;

        // registry for channels and its subscription
        registry reg;

        // create an acceptor for publishers, run it as fiber
        boost::fibers::asio::spawn( io_service,
            boost::bind( accept_publisher,
                boost::ref( io_service), 9997, boost::ref( reg), _1) );

        // create an acceptor for subscribersm, run it as fiber
        boost::fibers::asio::spawn( io_service,
            boost::bind( accept_subscriber,
                boost::ref( io_service), 9998, boost::ref( reg), _1) );
                
        boost::fibers::fiber f(
            boost::bind( boost::fibers::asio::run_service, boost::ref( io_service) ) );
        f.join();
    }
    catch ( std::exception const& e)
    { std::cerr << "Exception: " << e.what() << "\n"; }

    return 0;
}
