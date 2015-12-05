//
// promise_handler.hpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// modified by Oliver Kowalke and Nat Goodspeed
//

#ifndef BOOST_FIBERS_ASIO_DETAIL_PROMISE_HANDLER_HPP
#define BOOST_FIBERS_ASIO_DETAIL_PROMISE_HANDLER_HPP

#include <exception>

#include <boost/asio/handler_invoke_hook.hpp>
#include <boost/exception/all.hpp>

#include <boost/fiber/all.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace asio {
namespace detail {

// Completion handler to adapt a promise as a completion handler.
//[fibers_asio_promise_handler_base
template< typename T >
class promise_handler_base {
public:
    typedef std::shared_ptr< boost::fibers::promise< T > > promise_ptr;

    // Construct from any promise_completion_token subclass special value.
    template< typename Allocator >
    promise_handler_base( boost::fibers::asio::promise_completion_token< Allocator > const& pct) :
        promise_( std::make_shared< boost::fibers::promise< T > >(
                      std::allocator_arg, pct.get_allocator() ) )
//<-
        , ecp_( pct.ec_)
//->
    {}

    bool should_set_value( boost::system::error_code const& ec) {
        if ( ! ec) {
            // whew, success
            return true;
        }

//<-
        // ec indicates error
        if ( ecp_) {
            // promise_completion_token bound an error_code variable: set it
            * ecp_ = ec;
            // This is the odd case: although there's an error, user code
            // expressly forbid us to call set_exception(). We've set the
            // bound error code -- but future::get() will wait forever unless
            // we kick the promise SOMEHOW. Tell subclass to call set_value()
            // anyway.
            return true;
        }
//->
        // no bound error_code: cause promise_ to throw an exception
        promise_->set_exception(
                std::make_exception_ptr(
                    boost::system::system_error( ec) ) );
        // caller should NOT call set_value()
        return false;
    }

    promise_ptr get_promise() const {
        return promise_;
    }

private:
    promise_ptr                 promise_;
//<-
    boost::system::error_code * ecp_;
//->
};
//]

// generic promise_handler for arbitrary value
//[fibers_asio_promise_handler
template< typename T >
class promise_handler : public promise_handler_base< T > {
private:
//<-
    using promise_handler_base< T >::should_set_value;

//->
public:
    // Construct from any promise_completion_token subclass special value.
    template< typename Allocator >
    promise_handler( boost::fibers::asio::promise_completion_token< Allocator > const& pct) :
        promise_handler_base< T >( pct) {
    }

//<-
    void operator()( T t) {
        get_promise()->set_value( t);
    }
//->
    void operator()( boost::system::error_code const& ec, T t) {
        if ( should_set_value( ec) ) {
            get_promise()->set_value( t);
        }
    }
//<-
    using typename promise_handler_base< T >::promise_ptr;
    using promise_handler_base< T >::get_promise;
//->
};
//]

// specialize promise_handler for void
template<>
class promise_handler< void > : public promise_handler_base< void > {
private:
    using promise_handler_base< void >::should_set_value;

public:
    // Construct from any promise_completion_token subclass special value.
    template< typename Allocator >
    promise_handler( boost::fibers::asio::promise_completion_token< Allocator > const& pct) :
        promise_handler_base< void >( pct) {
    }

    void operator()() {
        get_promise()->set_value();
    }

    void operator()( boost::system::error_code const& ec) {
        if ( should_set_value( ec) ) {
            get_promise()->set_value();
        }
    }

    using promise_handler_base< void >::promise_ptr;
    using promise_handler_base< void >::get_promise;
};

}}}

namespace asio {
namespace detail {

// Specialize asio_handler_invoke hook to ensure that any exceptions thrown
// from the handler are propagated back to the caller via the future.
template< typename Function, typename T >
void asio_handler_invoke( Function f, fibers::asio::detail::promise_handler< T > * h) {
    typename fibers::asio::detail::promise_handler< T >::promise_ptr
        p( h->get_promise() );
    try {
        f();
    } catch (...) {
        p->set_exception( std::current_exception() );
    }
}

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_ASIO_DETAIL_PROMISE_HANDLER_HPP
