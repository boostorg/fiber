//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef WORKSTEALING_ROUND_ROBIN_H
#define WORKSTEALING_ROUND_ROBIN_H

#include <list>
#include <mutex>

#include <boost/config.hpp>

#include <boost/fiber/all.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

class workstealing_round_robin : public boost::fibers::sched_algorithm
{
private:
    typedef std::list< boost::fibers::fiber_handle >  rqueue_t;

    boost::fibers::mutex                mtx_;
    rqueue_t                  rqueue_;

public:
    virtual void awakened( boost::fibers::fiber_handle &);

    virtual boost::fibers::fiber_handle pick_next();

    boost::fibers::fiber steal();
};

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // WORKSTEALING_ROUND_ROBIN_H
