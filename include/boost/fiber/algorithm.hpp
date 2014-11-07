//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_ALGORITHM_H
#define BOOST_FIBERS_ALGORITHM_H

#include <boost/config.hpp>
#include <boost/utility.hpp>

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

struct sched_algorithm
{
    virtual ~sched_algorithm() {}

    virtual void awakened( detail::worker_fiber *) = 0;

    virtual detail::worker_fiber * pick_next() = 0;

    virtual void priority( detail::worker_fiber *, int) BOOST_NOEXCEPT = 0;

    virtual void property_change( detail::worker_fiber *, fiber_properties* ) {}
};

namespace detail {
// support sched_algorithm_with_properties::properties(fiber::id)
inline
fiber_base* extract_base(fiber_base::id id) { return id.impl_; }
} // detail

template <class PROPS>
struct sched_algorithm_with_properties: public sched_algorithm
{
public:
    typedef sched_algorithm_with_properties<PROPS> super;

    // Start every subclass awakened() override with:
    // sched_algorithm_with_properties<PROPS>::awakened(f);
    virtual void awakened( detail::worker_fiber *f)
    {
        if (! f->get_properties())
        {
            // TODO: would be great if PROPS could be allocated on the new
            // fiber's stack somehow
            f->set_properties(new PROPS(f, this));
        }
    }

    // used for all internal calls
    PROPS& properties(detail::worker_fiber* f)
    {
        return static_cast<PROPS&>(*f->get_properties());
    }

    // public-facing properties(fiber::id) method in case consumer retains a
    // pointer to supplied sched_algorithm_with_properties<PROPS> subclass
    PROPS& properties(detail::worker_fiber::id id)
    {
        return properties(extract(id));
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
