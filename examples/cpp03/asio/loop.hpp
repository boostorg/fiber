
//          Copyright Eugene Yakubovich 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/asio.hpp>
#include <boost/asio/high_resolution_timer.hpp>
#include <boost/bind.hpp>
#include <boost/chrono/system_clocks.hpp>
#include <boost/config.hpp>
#include <boost/ref.hpp>

#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/scheduler.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace asio {

inline void timer_handler( boost::asio::high_resolution_timer & timer) {
    boost::fibers::fm_yield();
    timer.expires_at( boost::fibers::fm_next_wakeup() );
    timer.async_wait( boost::bind( timer_handler, boost::ref( timer) ) );
}

inline void run_service( boost::asio::io_service & io_service) {
    boost::asio::high_resolution_timer timer( io_service, boost::chrono::seconds(0) );
    timer.async_wait( boost::bind( timer_handler, boost::ref( timer) ) );
    io_service.run();
}

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
