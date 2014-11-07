//          Copyright Nat Goodspeed 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// Define fiber_properties, a base class from which a library consumer can
// derive a subclass with specific properties important to a user-coded
// scheduler.

#ifndef BOOST_FIBERS_PROPERTIES_HPP
#define BOOST_FIBERS_PROPERTIES_HPP

#include <boost/fiber/detail/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

# if defined(BOOST_MSVC)
# pragma warning(push)
# pragma warning(disable:4275)
# endif

namespace boost {
namespace fibers {

namespace detail {
class worker_fiber;
} // detail

struct sched_algorithm;

class fiber_properties
{
protected:
    detail::worker_fiber* fiber_;
    sched_algorithm* sched_algo_;

    // Inform the relevant sched_algorithm instance that something important
    // has changed, so it can (presumably) adjust its data structures
    // accordingly.
    void notify()
    {
        sched_algo_->property_change(fiber_, this);
    }

public:
    // fiber_properties, and by implication every subclass, must accept back
    // pointers to its worker_fiber and the associated sched_algorithm*. Any
    // specific property setter method, after updating the relevant instance
    // variable, can/should call notify().
    fiber_properties(detail::worker_fiber* f, sched_algorithm* algo):
        fiber_(f),
        sched_algo_(algo)
    {}

    // We need a virtual destructor (hence a vtable) because fiber_properties
    // is stored polymorphically (as fiber_properties*) in worker_fiber, and
    // destroyed via that pointer.
    ~fiber_properties() {}
};

}} // namespace boost::fibers

# if defined(BOOST_MSVC)
# pragma warning(pop)
# endif

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_PROPERTIES_HPP
