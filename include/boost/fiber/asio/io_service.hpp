
//   Copyright Oliver Kowalke, Christopher M. Kohlhoff 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_ASIO_IO_SERVICE_HPP
#define BOOST_FIBERS_ASIO_IO_SERVICE_HPP

#include <deque>

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
    typedef std::deque< detail::fiber_base::ptr_t >     wqueue_t;
    typedef std::deque< boost::asio::io_service::work > wqueue_work_t;

    boost::asio::io_service&    io_service_;
    detail::fiber_base::ptr_t   active_fiber_;
    wqueue_t                    wqueue_;
    wqueue_work_t               wqueue_work_;

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

    void yield();
};

}}}

#endif // BOOST_FIBERS_ASIO_IO_SERVICE_HPP
