//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_ALGORITHM_H
#define BOOST_FIBERS_ALGORITHM_H

#include <boost/config.hpp>

#include <boost/fiber/properties.hpp>
#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/fiber_context.hpp>
 
#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

struct BOOST_FIBERS_DECL sched_algorithm {
    virtual ~sched_algorithm() {}

    virtual void awakened( fiber_context *) = 0;

    virtual fiber_context * pick_next() = 0;
};

namespace detail {
// support sched_algorithm_with_properties::properties(fiber::id)
inline
fiber_context* extract_context(fiber_context::id id) { return id.impl_; }
} // detail

struct BOOST_FIBERS_DECL sched_algorithm_with_properties_base: public sched_algorithm
{
    // called by fiber_properties::notify() -- don't directly call
    virtual void property_change_( fiber_context *f, fiber_properties* props ) = 0;
};

template <class PROPS>
struct sched_algorithm_with_properties:
        public sched_algorithm_with_properties_base
{
    typedef sched_algorithm_with_properties<PROPS> super;

    // Mark this override 'final': sched_algorithm_with_properties subclasses
    // must override awakened_props() instead. Otherwise you'd have to
    // remember to start every subclass awakened() override with:
    // sched_algorithm_with_properties<PROPS>::awakened(fb);
    virtual void awakened( fiber_context *f) final
    {
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

        // Okay, now forward the call to subclass override.
        awakened_props(fb);
    }

    // subclasses override this method instead of the original awakened()
    virtual void awakened_props( fiber_context *) = 0;

    // used for all internal calls
    PROPS& properties(fiber_context* f)
    {
        return static_cast<PROPS&>(*f->get_properties());
    }

    // public-facing properties(fiber::id) method in case consumer retains a
    // pointer to supplied sched_algorithm_with_properties<PROPS> subclass
    PROPS& properties(fiber::id id)
    {
        return properties(detail::extract_context(id));
    }

    // override this to be notified by PROPS::notify()
    virtual void property_change( fiber_context* f, PROPS& props) {}

    // implementation for sched_algorithm_with_properties_base method
    void property_change_( fiber_context *f, fiber_properties* props ) final
    {
        property_change(f, *static_cast<PROPS*>(props));
    }
};

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_ALGORITHM_H
