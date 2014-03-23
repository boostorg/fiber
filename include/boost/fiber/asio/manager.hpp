//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_ASIO_MANAGER_H
#define BOOST_FIBERS_ASIO_MANAGER_H

#include <boost/config.hpp>
#include <boost/asio.hpp>

#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/fiber_manager.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

# if defined(BOOST_MSVC)
# pragma warning(push)
# pragma warning(disable:4251 4275)
# endif

namespace boost {
namespace fibers {
namespace asio {

struct manager : public fiber_manager
{
    manager( boost::asio::io_service &) BOOST_NOEXCEPT;

    void run();

private:
    boost::asio::io_service   &   io_svc_;

    void evaluate_( detail::worker_fiber *);
};

}}}

# if defined(BOOST_MSVC)
# pragma warning(pop)
# endif

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_ASIO_MANAGER_H
