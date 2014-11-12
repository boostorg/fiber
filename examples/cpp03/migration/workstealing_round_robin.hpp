//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef WORKSTEALING_ROUND_ROBIN_H
#define WORKSTEALING_ROUND_ROBIN_H

#include <deque>

#include <boost/config.hpp>
#include <boost/thread/mutex.hpp>

#include <boost/fiber/fiber.hpp>
#include <boost/fiber/properties.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

# if defined(BOOST_MSVC)
# pragma warning(push)
# pragma warning(disable:4251 4275)
# endif

struct affinity: public boost::fibers::fiber_properties
{
    affinity(boost::fibers::fiber_properties::back_ptr p):
        fiber_properties(p),
        // By default, assume a given fiber CAN migrate to another thread.
        thread_affinity(false)
    {}

    bool thread_affinity;
};

class workstealing_round_robin :
    public boost::fibers::sched_algorithm_with_properties<affinity>
{
private:
    boost::mutex                mtx_;

    // We should package these as a queue class. Better yet, we should
    // refactor one of our existing (intrusive) queue classes to support the
    // required operations generically. But for now...
    boost::fibers::fiber_base *rhead_, **rtail_;

public:
    workstealing_round_robin();

    virtual void awakened( boost::fibers::fiber_base *);

    virtual boost::fibers::fiber_base * pick_next();

    boost::fibers::fiber steal();
};

# if defined(BOOST_MSVC)
# pragma warning(pop)
# endif

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // WORKSTEALING_ROUND_ROBIN_H
