
//   Copyright Oliver Kowalke, Christopher M. Kohlhoff 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_ASIO_IO_SERVICE_HPP
#define BOOST_FIBERS_ASIO_IO_SERVICE_HPP

#include <deque>
#include <utility>

#include <boost/asio/io_service.hpp>
#include <boost/config.hpp>

#include <boost/fiber/algorithm.hpp>
#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/fiber_base.hpp>

namespace boost {
namespace fibers {
namespace asio {

class BOOST_FIBERS_DECL io_service : public algorithm
{
private:
    struct schedulable
    {
        detail::fiber_base::ptr_t       f;
        clock_type::time_point          tp;
        boost::asio::io_service::work   w;

        schedulable( detail::fiber_base::ptr_t const& f_,
                     clock_type::time_point const& tp_,
                     boost::asio::io_service::work const& w_) :
            f( f_), tp( tp_), w( w_)
        { BOOST_ASSERT( f); }

        schedulable( detail::fiber_base::ptr_t const& f_,
                     boost::asio::io_service::work const& w_) :
            f( f_), tp( (clock_type::time_point::max)() ), w( w_)
        { BOOST_ASSERT( f); }
    };

    typedef std::deque< schedulable >                   wqueue_t;

    boost::asio::io_service &   io_service_;
    detail::fiber_base::ptr_t   active_fiber_;
    wqueue_t                    wqueue_;

    void evaluate_( detail::fiber_base::ptr_t const&);

public:
    io_service( boost::asio::io_service & svc) BOOST_NOEXCEPT;

    ~io_service() BOOST_NOEXCEPT;

    void spawn( detail::fiber_base::ptr_t const&);

    void priority( detail::fiber_base::ptr_t const&, int);

    void join( detail::fiber_base::ptr_t const&);

    detail::fiber_base::ptr_t active() BOOST_NOEXCEPT
    { return active_fiber_; }

    bool run();

    void wait();
    bool wait_until( clock_type::time_point const&);

    void yield();
};

}}}

#endif // BOOST_FIBERS_ASIO_IO_SERVICE_HPP
