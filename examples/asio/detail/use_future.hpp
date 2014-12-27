//
// use_future.hpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// modified by Oliver Kowalke
//

#ifndef BOOST_FIBERS_ASIO_DETAIL_USE_FUTURE_HPP
#define BOOST_FIBERS_ASIO_DETAIL_USE_FUTURE_HPP

#include <memory>

#include <boost/asio/async_result.hpp>
#include <boost/asio/detail/config.hpp>
#include <boost/asio/handler_type.hpp>

#include <boost/fiber/future/future.hpp>
#include <boost/fiber/future/promise.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace asio {
namespace detail {

// Completion handler to adapt a promise as a completion handler.
template< typename T >
class promise_handler
{
public:
    // Construct from use_future special value.
    template< typename Allocator >
    promise_handler( boost::fibers::asio::use_future_t< Allocator > uf) :
        promise_( std::make_shared< boost::fibers::promise< T > >(
            uf.get_allocator(), std::allocator_arg, uf.get_allocator() ) )
    {}

    void operator()( T t)
    {
        promise_->set_value( t);
        //boost::fibers::fm_run();
    }

    void operator()( std::error_code const& ec, T t)
    {
        if (ec)
            promise_->set_exception(
                    boost::copy_exception(
                        boost::system::system_error( ec) ) );
        else
            promise_->set_value( t);

        // scheduler::run() resumes a ready fiber
        // invoke scheduler::run() until no fiber was resumed
        //boost::fibers::fm_run();
    }

    //private:
    boost::shared_ptr< boost::fibers::promise< T > >    promise_;
};

// Completion handler to adapt a void promise as a completion handler.
template<>
class promise_handler< void >
{
public:
    // Construct from use_future special value. Used during rebinding.
    template< typename Allocator >
    promise_handler( boost::fibers::asio::use_future_t< Allocator > uf) :
        promise_( boost::allocate_shared< boost::fibers::promise< void > >(
            uf.get_allocator(), boost::allocator_arg, uf.get_allocator() ) )
    {}

    void operator()()
    {
        promise_->set_value();
        //boost::fibers::fm_run();
    }

    void operator()( boost::system::error_code const& ec)
    {
        if ( ec)
            promise_->set_exception(
                    boost::copy_exception(
                        boost::system::system_error( ec) ) );
        else
            promise_->set_value();

        // scheduler::run() resumes a ready fiber
        // invoke scheduler::run() until no fiber was resumed
        //boost::fibers::fm_run();
    }

    //private:
    boost::shared_ptr< boost::fibers::promise< void > > promise_;
};

// Ensure any exceptions thrown from the handler are propagated back to the
// caller via the future.
template< typename Function, typename T >
void asio_handler_invoke( Function f, promise_handler< T > * h)
{
    boost::shared_ptr< boost::fibers::promise< T > > p( h->promise_);
    try
    { f(); }
    catch (...)
    { p->set_exception( boost::current_exception() ); }
}

} // namespace detail

#if !defined(GENERATING_DOCUMENTATION)

// Handler traits specialisation for promise_handler.
template< typename T >
class async_result< detail::promise_handler< T > >
{
public:
    // The initiating function will return a future.
    typedef boost::fibers::future< T >  type;

    // Constructor creates a new promise for the async operation, and obtains the
    // corresponding future.
    explicit async_result( detail::promise_handler< T > & h)
    { value_ = h.promise_->get_future(); }

    // Obtain the future to be returned from the initiating function.
    type get()
    { return boost::move( value_); }

private:
    type    value_;
};

// Handler type specialisation for use_future.
template< typename Allocator, typename ReturnType >
struct handler_type<
    boost::fibers::asio::use_future_t< Allocator>,
    ReturnType()
>
{ typedef detail::promise_handler< void >   type; };

// Handler type specialisation for use_future.
template< typename Allocator, typename ReturnType, typename Arg1 >
struct handler_type<
    boost::fibers::asio::use_future_t< Allocator >,
    ReturnType( Arg1)
>
{ typedef detail::promise_handler< Arg1 > type; };

// Handler type specialisation for use_future.
template< typename Allocator, typename ReturnType >
struct handler_type<
    boost::fibers::asio::use_future_t< Allocator >,
    ReturnType( boost::system::error_code)
>
{ typedef detail::promise_handler< void >   type; };

// Handler type specialisation for use_future.
template< typename Allocator, typename ReturnType, typename Arg2 >
struct handler_type<
    boost::fibers::asio::use_future_t< Allocator >,
    ReturnType( boost::system::error_code, Arg2)
>
{ typedef detail::promise_handler< Arg2 >   type; };

#endif // !defined(GENERATING_DOCUMENTATION)

} // namespace asio
} // namespace boost

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_ASIO_DETAIL_USE_FUTURE_HPP
