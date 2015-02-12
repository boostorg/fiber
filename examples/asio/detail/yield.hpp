#ifndef BOOST_FIBERS_ASIO_DETAIL_YIELD_HPP
#define BOOST_FIBERS_ASIO_DETAIL_YIELD_HPP

#include <boost/asio/async_result.hpp>
#include <boost/asio/detail/config.hpp>
#include <boost/asio/handler_type.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <boost/throw_exception.hpp>

#include <boost/fiber/all.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace asio {
namespace detail {

template< typename T >
class yield_handler
{
public:
    yield_handler( yield_t const& y) :
        fiber_( boost::fibers::detail::scheduler::instance()->active() ),
        ec_( y.ec_), value_( 0)
    {}

    void operator()( T t)
    {
        * ec_ = boost::system::error_code();
        * value_ = t;
        fiber_->set_ready();
    }

    void operator()( boost::system::error_code const& ec, T t)
    {
        * ec_ = ec;
        * value_ = t;
        fiber_->set_ready();
    }

//private:
    boost::fibers::fiber_context       *   fiber_;
    boost::system::error_code               *   ec_;
    T                                       *   value_;
};

// Completion handler to adapt a void promise as a completion handler.
template<>
class yield_handler< void >
{
public:
    yield_handler( yield_t const& y) :
        fiber_( boost::fibers::detail::scheduler::instance()->active() ),
        ec_( y.ec_)
    {}
    
    void operator()()
    {
        * ec_ = boost::system::error_code();
        fiber_->set_ready();
    }
    
    void operator()( boost::system::error_code const& ec)
    {
        * ec_ = ec;
        fiber_->set_ready();
    }

//private:
    boost::fibers::fiber_context     *   fiber_;
    boost::system::error_code             *   ec_;
};

} // namespace detail
} // namespace asio
} // namespace fibers
} // namespace boost

namespace boost {
namespace asio {

template< typename T >
class async_result< boost::fibers::asio::detail::yield_handler< T > >
{
public:
    typedef T type;
    
    explicit async_result( boost::fibers::asio::detail::yield_handler< T > & h)
    {
        out_ec_ = h.ec_;
        if ( ! out_ec_) h.ec_ = & ec_;
        h.value_ = & value_;
    }
    
    type get()
    {
        fibers::detail::spinlock splk;
        std::unique_lock< fibers::detail::spinlock > lk( splk);
        boost::fibers::detail::scheduler::instance()->wait(lk);
        if ( ! out_ec_ && ec_)
            throw_exception( boost::system::system_error( ec_) );
        return value_;
    }

private:
    boost::system::error_code *   out_ec_;
    boost::system::error_code     ec_;
    type                          value_;
};

template<>
class async_result< boost::fibers::asio::detail::yield_handler< void > >
{
public:
    typedef void  type;

    explicit async_result( boost::fibers::asio::detail::yield_handler< void > & h)
    {
        out_ec_ = h.ec_;
        if ( ! out_ec_) h.ec_ = & ec_;
    }

    void get()
    {
        fibers::detail::spinlock splk;
        std::unique_lock< fibers::detail::spinlock > lk( splk);
        boost::fibers::detail::scheduler::instance()->wait(lk);
        if ( ! out_ec_ && ec_)
            throw_exception( boost::system::system_error( ec_) );
    }

private:
    boost::system::error_code *   out_ec_;
    boost::system::error_code     ec_;
};

// Handler type specialisation for use_future.
template< typename ReturnType >
struct handler_type<
    boost::fibers::asio::yield_t,
    ReturnType()
>
{ typedef boost::fibers::asio::detail::yield_handler< void >    type; };

// Handler type specialisation for use_future.
template< typename ReturnType, typename Arg1 >
struct handler_type<
    boost::fibers::asio::yield_t,
    ReturnType( Arg1)
>
{ typedef boost::fibers::asio::detail::yield_handler< Arg1 >    type; };

// Handler type specialisation for use_future.
template< typename ReturnType >
struct handler_type<
    boost::fibers::asio::yield_t,
    ReturnType( boost::system::error_code)
>
{ typedef boost::fibers::asio::detail::yield_handler< void >    type; };

// Handler type specialisation for use_future.
template< typename ReturnType, typename Arg2 >
struct handler_type<
    boost::fibers::asio::yield_t,
    ReturnType( boost::system::error_code, Arg2)
>
{ typedef boost::fibers::asio::detail::yield_handler< Arg2 >    type; };

} // namespace asio
} // namespace boost

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_ASIO_DETAIL_YIELD_HPP
