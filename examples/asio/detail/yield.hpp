#ifndef BOOST_FIBERS_ASIO_DETAIL_YIELD_HPP
#define BOOST_FIBERS_ASIO_DETAIL_YIELD_HPP

#include <boost/asio/async_result.hpp>
#include <boost/asio/detail/config.hpp>
#include <boost/asio/handler_type.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <boost/throw_exception.hpp>

#include <boost/fiber/all.hpp>

#include <mutex>                    // std::unique_lock

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace asio {
namespace detail {

// This class encapsulates common elements between yield_handler<T> (capturing
// a value to return from asio async function) and yield_handler<void> (no
// such value). See yield_handler<T> and its <void> specialization below.
class yield_handler_base {
public:
    yield_handler_base( yield_base const& y) :
        // capture the context* associated with the running fiber
        ctx_( boost::fibers::context::active() ),
        // capture the passed yield_base
        yb_( y )
    {}

    // completion callback passing only (error_code)
    void operator()( boost::system::error_code const& ec)
    {
        // We can't afford a wait() call during this method.
        std::unique_lock< fibers::mutex > lk( mtx_ );
        // Notify a subsequent wait() call that it need not suspend.
        completed_ = true;
        // set the error_code bound by yield_t
        * yb_.ec_ = ec;
        // Are we permitted to wake up the suspended fiber on this thread, the
        // thread that called the completion handler?
        if ( ctx_->is_context( pinned_context ) || ( ! yb_.allow_hop_ )) {
            // We must not migrate a pinned_context to another thread.
            // If the application passed yield_t rather than yield_hop_t, we
            // are forbidden to migrate this fiber.
            // Just wake this fiber on its original scheduler.
            ctx_->set_ready( ctx_);
        } else {
            // seems okay to migrate this context
            // migrate the waiting fiber to the running thread
            boost::fibers::context::active()->migrate( ctx_);
            // and wake the fiber
            boost::fibers::context::active()->set_ready( ctx_);
        }
    }

    void wait()
    {
        // do not interleave operator() with wait()
        std::unique_lock< fibers::mutex > lk( mtx_ );
        // if operator() has not yet been called
        if ( ! completed_ ) {
            // then permit it to be called
            lk.unlock();
            // and wait for that call
            fibers::context::active()->suspend();
        }
    }

//private:
    boost::fibers::context      *   ctx_;
    yield_base                      yb_;

private:
    // completed_ tracks whether complete() is called before get()
    fibers::mutex                   mtx_;
    bool                            completed_{ false };
};

// handler_type< yield_t, ... >::type is yield_handler<>. When you pass an
// instance of yield_t as an asio completion token, asio uses
// handler_type<>::type to select yield_handler<> as the actual handler class.
template< typename T >
class yield_handler: public yield_handler_base
{
public:
    // asio passes the completion token to the handler constructor
    explicit yield_handler( yield_base const& y) :
        yield_handler_base( y),
        // pointer to destination for eventual value
        value_( nullptr)
    {}

    // completion callback passing only value (T)
    void operator()( T t)
    {
        // just like a callback passing success error_code
        (*this)( boost::system::error_code(), std::move(t) );
    }

    // completion callback passing (error_code, T)
    void operator()( boost::system::error_code const& ec, T t)
    {
        // move the value to async_result<> instance
        * value_ = std::move( t);
        // forward the call to base-class completion handler
        yield_handler_base::operator()( ec);
    }

//private:
    T                           *   value_;
};

// yield_handler<void> is like yield_handler<T> without value_. In fact it's
// just like yield_handler_base.
template<>
class yield_handler< void >: public yield_handler_base
{
public:
    explicit yield_handler( yield_base const& y) :
        yield_handler_base( y)
    {}

    // nullary completion callback
    void operator()()
    {
        (*this)( boost::system::error_code() );
    }

    // inherit operator()(error_code) overload from base class
    using yield_handler_base::operator();

};

// Specialize asio_handler_invoke hook to ensure that any exceptions thrown
// from the handler are propagated back to the caller
template< typename Fn, typename T >
void asio_handler_invoke( Fn fn, yield_handler< T > * h) {
        fn();
}

} // namespace detail
} // namespace asio
} // namespace fibers
} // namespace boost

namespace boost {
namespace asio {

class async_result_base
{
public:
    template< typename T >
    explicit async_result_base( boost::fibers::asio::detail::yield_handler_base & h):
        // bind this yield_handler_base for later use
        yh_( h)
    {
        // if yield_t didn't bind an error_code, make yield_handler_base's
        // error_code* point to an error_code local to this object so
        // yield_handler_base::operator() can unconditionally store through
        // its error_code*
        if ( ! h.yb_.ec_) {
            h.yb_.ec_ = & ec_;
        }
    }

    void get() {
        // Unless yield_handler_base::operator() has already been called,
        // suspend the calling fiber until that call.
        yh_.wait();
        // The only way our own ec_ member could have a non-default value is
        // if our yield_handler did not have a bound error_code AND the
        // completion callback passed a non-default error_code.
        if ( ec_) {
            throw_exception( boost::system::system_error( ec_) );
        }
        boost::this_fiber::interruption_point();
    }

protected:
    boost::system::error_code ec_;

private:
    fibers::asio::detail::yield_handler_base& yh_;
};

// asio constructs an async_result<> instance from the yield_handler specified
// by handler_type<>::type. A particular asio async method constructs the
// yield_handler, constructs this async_result specialization from it, then
// returns the result of calling its get() method.
template< typename T >
class async_result< boost::fibers::asio::detail::yield_handler< T > >:
    public async_result_base {
public:
    // type returned by get()
    typedef T type;

    // async_result is constructed with a non-const yield_handler instance
    explicit async_result( boost::fibers::asio::detail::yield_handler< T > & h):
        async_result_base(h)
    {
        // set yield_handler's T* to this object's value_ member so
        // yield_handler::operator() will store the callback value here
        h.value_ = & value_;
    }

    // asio async method returns result of calling get()
    type get() {
        async_result_base::get();
        return std::move( value_);
    }

private:
    type                          value_;
};

// Without the need to handle a passed value, our yield_handler<void>
// specialization is just like async_result_base.
template<>
class async_result< boost::fibers::asio::detail::yield_handler< void > >:
    public async_result_base {
public:
    typedef void  type;

    explicit async_result( boost::fibers::asio::detail::yield_handler< void > & h):
        async_result_base(h)
    {}
};

// Handler type specialisation for fibers::asio::yield.
// When 'yield' is passed as a completion handler which accepts no parameters,
// use yield_handler<void>.
template< typename ReturnType >
struct handler_type< fibers::asio::yield_base, ReturnType() >
{ typedef fibers::asio::detail::yield_handler< void >    type; };
template< typename ReturnType >
struct handler_type< fibers::asio::yield_t, ReturnType() >
{ typedef fibers::asio::detail::yield_handler< void >    type; };
template< typename ReturnType >
struct handler_type< fibers::asio::yield_hop_t, ReturnType() >
{ typedef fibers::asio::detail::yield_handler< void >    type; };

// Handler type specialisation for fibers::asio::yield.
// When 'yield' is passed as a completion handler which accepts a data
// parameter, use yield_handler<parameter type> to return that parameter to
// the caller.
template< typename ReturnType, typename Arg1 >
struct handler_type< fibers::asio::yield_base, ReturnType( Arg1) >
{ typedef fibers::asio::detail::yield_handler< Arg1 >    type; };
template< typename ReturnType, typename Arg1 >
struct handler_type< fibers::asio::yield_t, ReturnType( Arg1) >
{ typedef fibers::asio::detail::yield_handler< Arg1 >    type; };
template< typename ReturnType, typename Arg1 >
struct handler_type< fibers::asio::yield_hop_t, ReturnType( Arg1) >
{ typedef fibers::asio::detail::yield_handler< Arg1 >    type; };

// Handler type specialisation for fibers::asio::yield.
// When 'yield' is passed as a completion handler which accepts only
// error_code, use yield_handler<void>. yield_handler will take care of the
// error_code one way or another.
template< typename ReturnType >
struct handler_type< fibers::asio::yield_base, ReturnType( boost::system::error_code) >
{ typedef fibers::asio::detail::yield_handler< void >    type; };
template< typename ReturnType >
struct handler_type< fibers::asio::yield_t, ReturnType( boost::system::error_code) >
{ typedef fibers::asio::detail::yield_handler< void >    type; };
template< typename ReturnType >
struct handler_type< fibers::asio::yield_hop_t, ReturnType( boost::system::error_code) >
{ typedef fibers::asio::detail::yield_handler< void >    type; };

// Handler type specialisation for fibers::asio::yield.
// When 'yield' is passed as a completion handler which accepts a data
// parameter and an error_code, use yield_handler<parameter type> to return
// just the parameter to the caller. yield_handler will take care of the
// error_code one way or another.
template< typename ReturnType, typename Arg2 >
struct handler_type< fibers::asio::yield_base, ReturnType( boost::system::error_code, Arg2) >
{ typedef fibers::asio::detail::yield_handler< Arg2 >    type; };
template< typename ReturnType, typename Arg2 >
struct handler_type< fibers::asio::yield_t, ReturnType( boost::system::error_code, Arg2) >
{ typedef fibers::asio::detail::yield_handler< Arg2 >    type; };
template< typename ReturnType, typename Arg2 >
struct handler_type< fibers::asio::yield_hop_t, ReturnType( boost::system::error_code, Arg2) >
{ typedef fibers::asio::detail::yield_handler< Arg2 >    type; };

} // namespace asio
} // namespace boost

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_ASIO_DETAIL_YIELD_HPP
