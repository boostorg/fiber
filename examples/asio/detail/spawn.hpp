//
// detail/spawn.hpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_FIBERS_ASIO_DETAIL_SPAWN_HPP
#define BOOST_FIBERS_ASIO_DETAIL_SPAWN_HPP

#include <boost/asio/async_result.hpp>
#include <boost/asio/detail/config.hpp>
#include <boost/asio/detail/handler_alloc_helpers.hpp>
#include <boost/asio/detail/handler_cont_helpers.hpp>
#include <boost/asio/detail/handler_invoke_helpers.hpp>
#include <boost/asio/detail/noncopyable.hpp>
#include <boost/asio/detail/shared_ptr.hpp>
#include <boost/asio/handler_type.hpp>
#include <boost/config.hpp>

#include <boost/fiber/context.hpp>
#include <boost/fiber/operations.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace asio {
namespace detail {

template< typename Handler, typename T >
class fiber_handler {
public:
    fiber_handler( basic_yield_context< Handler > ctx) :
        ctx_( ctx.ctx_),
        handler_( ctx.handler_),
        ec_( ctx.ec_),
        value_( 0) {
    }

    void operator()( T value) {
        * ec_ = boost::system::error_code();
        * value_ = value;
        boost::fibers::context::active()->set_ready( ctx_);
    }

    void operator()( boost::system::error_code ec, T value) {
        * ec_ = ec;
        * value_ = value;
        boost::fibers::context::active()->set_ready( ctx_);
    }

//private:
    boost::fibers::context      *   ctx_;
    Handler                     &   handler_;
    boost::system::error_code   *   ec_;
    T                           *   value_;
};

template< typename Handler >
class fiber_handler< Handler, void  > {
public:
    fiber_handler( basic_yield_context< Handler > ctx) :
        ctx_( ctx.ctx_),
        handler_( ctx.handler_),
        ec_( ctx.ec_) {
    }

    void operator()() {
        * ec_ = boost::system::error_code();
        boost::fibers::context::active()->set_ready( ctx_);
    }

    void operator()( boost::system::error_code ec) {
        * ec_ = ec;
        boost::fibers::context::active()->set_ready( ctx_);
    }

//private:
    boost::fibers::context      *   ctx_;
    Handler                     &   handler_;
    boost::system::error_code   *   ec_;
};

template< typename Handler, typename T >
void* asio_handler_allocate( std::size_t size,
        fiber_handler< Handler, T > * this_handler) {
    return boost_asio_handler_alloc_helpers::allocate(
            size, this_handler->handler_);
}

template< typename Handler, typename T >
void asio_handler_deallocate( void* pointer, std::size_t size,
        fiber_handler< Handler, T > * this_handler) {
    boost_asio_handler_alloc_helpers::deallocate(
            pointer, size, this_handler->handler_);
}

template< typename Handler, typename T >
bool asio_handler_is_continuation( fiber_handler<Handler, T> *) {
    return true;
}

template< typename Function, typename Handler, typename T >
void asio_handler_invoke( Function & function,
        fiber_handler< Handler, T > * this_handler) {
    boost_asio_handler_invoke_helpers::invoke(
            function, this_handler->handler_);
}

template< typename Function, typename Handler, typename T >
void asio_handler_invoke( Function const& function,
        fiber_handler< Handler, T > * this_handler) {
    boost_asio_handler_invoke_helpers::invoke(
            function, this_handler->handler_);
}

}}}

namespace asio {

template< typename Handler, typename ReturnType >
struct handler_type<
    boost::fibers::asio::basic_yield_context< Handler >,
    ReturnType()
>
{ typedef boost::fibers::asio::detail::fiber_handler< Handler, void >    type; };

template< typename Handler, typename ReturnType, typename Arg1 >
struct handler_type<
    boost::fibers::asio::basic_yield_context< Handler >,
    ReturnType( Arg1)
>
{ typedef boost::fibers::asio::detail::fiber_handler< Handler, Arg1 >    type; };

template< typename Handler, typename ReturnType >
struct handler_type<
    boost::fibers::asio::basic_yield_context< Handler >,
    ReturnType( boost::system::error_code)
>
{ typedef boost::fibers::asio::detail::fiber_handler <Handler, void >    type; };

template< typename Handler, typename ReturnType, typename Arg2 >
struct handler_type<
    boost::fibers::asio::basic_yield_context< Handler >,
    ReturnType( boost::system::error_code, Arg2)
>
{ typedef boost::fibers::asio::detail::fiber_handler< Handler, Arg2 >    type; };

template< typename Handler, typename T >
class async_result< boost::fibers::asio::detail::fiber_handler< Handler, T > > {
public:
    typedef T type;

    explicit async_result( boost::fibers::asio::detail::fiber_handler< Handler, T > & h) :
        out_ec_( nullptr), ec_(), value_() {
        out_ec_ = h.ec_;
        if ( ! out_ec_) {
            h.ec_ = & ec_;
        }
        h.value_ = & value_;
    }

    type get() {
        boost::fibers::context::active()->suspend();
        if ( ! out_ec_ && ec_) {
            throw boost::system::system_error( ec_);
        }
        return value_;
    }

private:
    boost::system::error_code   *   out_ec_;
    boost::system::error_code       ec_;
    type                            value_;
};

template< typename Handler >
class async_result< boost::fibers::asio::detail::fiber_handler< Handler, void > > {
public:
    typedef void type;

    explicit async_result( boost::fibers::asio::detail::fiber_handler< Handler, void > & h) :
        out_ec_( nullptr), ec_() {
        out_ec_ = h.ec_;
        if ( ! out_ec_) {
            h.ec_ = &ec_;
        }
    }

    void get() {
        boost::fibers::context::active()->suspend();
        if ( ! out_ec_ && ec_) {
            throw boost::system::system_error( ec_);
        }
    }

private:
    boost::system::error_code   *   out_ec_;
    boost::system::error_code       ec_;
};

}

namespace fibers {
namespace asio {
namespace detail {

template< typename Handler, typename Function >
struct spawn_data : private noncopyable {
    spawn_data( boost::asio::io_service& io_svc, BOOST_ASIO_MOVE_ARG( Handler) handler,
            bool call_handler, BOOST_ASIO_MOVE_ARG( Function) function) :
        io_svc_(io_svc),
        handler_( BOOST_ASIO_MOVE_CAST( Handler)( handler) ),
        call_handler_( call_handler),
        function_( BOOST_ASIO_MOVE_CAST( Function)( function) ) {
    }

    boost::asio::io_service     &   io_svc_;
    boost::fibers::context      *   ctx_;
    Handler                         handler_;
    bool                            call_handler_;
    Function                        function_;
};

template< typename Handler, typename Function >
struct fiber_entry_point {
    void operator()() {
        shared_ptr< spawn_data< Handler, Function > > data( data_);
        data->ctx_ = boost::fibers::context::active();
        const basic_yield_context< Handler > yield(
                data->ctx_, data->handler_);

        boost::asio::io_service::work w( data->io_svc_);
        ( data->function_)( yield);
        if ( data->call_handler_) {
            ( data->handler_)();
        }
    }

    shared_ptr< spawn_data< Handler, Function > > data_;
};

template< typename Handler, typename Function >
struct spawn_helper {
    void operator()() {
        fiber_entry_point< Handler, Function > entry_point = { data_ };
        boost::fibers::fiber( entry_point).detach();
        boost::this_fiber::yield();
    }

    shared_ptr< spawn_data< Handler, Function > > data_;
};

inline void default_spawn_handler() {}

}

template< typename Handler, typename Function >
void spawn( boost::asio::io_service& io_service,
        BOOST_ASIO_MOVE_ARG( Handler) handler,
        BOOST_ASIO_MOVE_ARG( Function) function) {
    detail::spawn_helper< Handler, Function > helper;
    helper.data_.reset(
            new detail::spawn_data< Handler, Function >(
                io_service,
                BOOST_ASIO_MOVE_CAST( Handler)( handler), true,
                BOOST_ASIO_MOVE_CAST( Function)( function) ) );
    boost_asio_handler_invoke_helpers::invoke(
            helper, helper.data_->handler_);
}

template< typename Handler, typename Function >
void spawn( basic_yield_context< Handler > ctx,
        BOOST_ASIO_MOVE_ARG( Function) function) {
    Handler handler( ctx.handler_); // Explicit copy that might be moved from.
    detail::spawn_helper< Handler, Function > helper;
    helper.data_.reset(
            new detail::spawn_data< Handler, Function >(
                BOOST_ASIO_MOVE_CAST( Handler)( handler), false,
                BOOST_ASIO_MOVE_CAST( Function)( function) ) );
    boost_asio_handler_invoke_helpers::invoke(
            helper, helper.data_->handler_);
}

template< typename Function >
void spawn( boost::asio::io_service::strand strand,
        BOOST_ASIO_MOVE_ARG( Function) function) {
    boost::fibers::asio::spawn(
            strand.get_io_service(),
            strand.wrap( & detail::default_spawn_handler),
            BOOST_ASIO_MOVE_CAST( Function)( function));
}

template< typename Function >
void spawn( boost::asio::io_service & io_service,
        BOOST_ASIO_MOVE_ARG( Function) function) {
    boost::fibers::asio::spawn(
            boost::asio::io_service::strand( io_service),
            BOOST_ASIO_MOVE_CAST( Function)( function));
}

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_ASIO_DETAIL_SPAWN_HPP
