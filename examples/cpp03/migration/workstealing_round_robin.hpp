//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef WORKSTEALING_ROUND_ROBIN_H
#define WORKSTEALING_ROUND_ROBIN_H

#include <deque>

#include <boost/config.hpp>
#include <boost/thread/mutex.hpp>

#include <boost/fiber/all.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

# if defined(BOOST_MSVC)
# pragma warning(push)
# pragma warning(disable:4251 4275)
# endif

class workstealing_round_robin : public boost::fibers::sched_algorithm
{
private:
    typedef std::deque< boost::fibers::detail::fiber_base * >  rqueue_t;

    boost::mutex                mtx_;
    rqueue_t                    rqueue_;

public:
    virtual void awakened( boost::fibers::detail::fiber_base *);

    virtual boost::fibers::detail::fiber_base * pick_next();

    virtual void priority( boost::fibers::detail::fiber_base *, int) BOOST_NOEXCEPT;

    boost::fibers::fiber steal();
};

# if defined(BOOST_MSVC)
# pragma warning(pop)
# endif

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // WORKSTEALING_ROUND_ROBIN_H
