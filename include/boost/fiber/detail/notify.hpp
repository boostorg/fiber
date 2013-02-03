
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_DETAIL_NOTIFY_H
#define BOOST_FIBERS_DETAIL_NOTIFY_H

#include <cstddef>

#include <boost/assert.hpp>
#include <boost/atomic.hpp>
#include <boost/config.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/utility.hpp>

#include <boost/fiber/detail/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace detail {

class notify : private noncopyable
{
private:
    atomic< bool >          wake_up_;

protected:
    virtual void add_ref() BOOST_NOEXCEPT = 0;
    virtual void release_ref() = 0;

public:
    typedef intrusive_ptr< notify >     ptr_t;

    notify() :
        wake_up_( false)
    {}

    virtual ~notify() {};

    bool woken_up() BOOST_NOEXCEPT
    { return wake_up_.exchange( false, memory_order_seq_cst); }

    void wake_up() BOOST_NOEXCEPT
    { wake_up_.exchange( true, memory_order_seq_cst); }

    friend inline void intrusive_ptr_add_ref( notify * p) BOOST_NOEXCEPT
    { p->add_ref(); }

    friend inline void intrusive_ptr_release( notify * p)
    { p->release_ref(); }
};

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DETAIL_NOTIFY_H
