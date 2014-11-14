//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_ALGORITHM_H
#define BOOST_FIBERS_ALGORITHM_H

#include <boost/config.hpp>
#include <boost/utility.hpp>

#include <boost/fiber/properties.hpp>
#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/worker_fiber.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

# if defined(BOOST_MSVC)
# pragma warning(push)
# pragma warning(disable:4251 4275)
# endif

namespace boost {
namespace fibers {

// hoist fiber_base out of detail namespace into boost::fibers
typedef detail::fiber_base fiber_base;

struct sched_algorithm
{
    virtual ~sched_algorithm() {}

    virtual void awakened( fiber_base *) = 0;

    virtual fiber_base * pick_next() = 0;
};

namespace detail {
// support sched_algorithm_with_properties::properties(fiber::id)
inline
fiber_base* extract_base(fiber_base::id id) { return id.impl_; }
} // detail

struct sched_algorithm_with_properties_base: public sched_algorithm
{
    // called by fiber_properties::notify() -- don't directly call
    virtual void property_change_( fiber_base *f, fiber_properties* props ) = 0;
};

template <class PROPS>
struct sched_algorithm_with_properties:
        public sched_algorithm_with_properties_base
{
public:
    typedef sched_algorithm_with_properties<PROPS> super;

    // Start every subclass awakened() override with:
    // sched_algorithm_with_properties<PROPS>::awakened(fb);
    virtual void awakened( fiber_base *fb)
    {
        detail::worker_fiber* f = static_cast<detail::worker_fiber*>(fb);
        if (! f->get_properties())
        {
            // TODO: would be great if PROPS could be allocated on the new
            // fiber's stack somehow
            f->set_properties(new PROPS(f));
        }
        // Set sched_algo_ again every time this fiber becomes READY. That
        // handles the case of a fiber migrating to a new thread with a new
        // sched_algorithm subclass instance.
        f->get_properties()->set_sched_algorithm(this);
    }

    // used for all internal calls
    PROPS& properties(fiber_base* f)
    {
        return static_cast<PROPS&>(*static_cast<detail::worker_fiber*>(f)->get_properties());
    }

    // public-facing properties(fiber::id) method in case consumer retains a
    // pointer to supplied sched_algorithm_with_properties<PROPS> subclass
    PROPS& properties(detail::worker_fiber::id id)
    {
        return properties(extract(id));
    }

    // override this to be notified by PROPS::notify()
    virtual void property_change( fiber_base* f, PROPS& props) {}

    // implementation for sched_algorithm_with_properties_base method
    void property_change_( fiber_base *f, fiber_properties* props )
    {
        property_change(f, *static_cast<PROPS*>(props));
    }

private:
    // support sched_algorithm_with_properties::properties(fiber::id)
    detail::worker_fiber* extract(detail::worker_fiber::id id)
    {
        return static_cast<detail::worker_fiber*>(detail::extract_base(id));
    }
};

}}

# if defined(BOOST_MSVC)
# pragma warning(pop)
# endif

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_ALGORITHM_H
