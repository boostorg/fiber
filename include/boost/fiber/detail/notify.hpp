
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_DETAIL_NOTIFY_H
#define BOOST_FIBERS_DETAIL_NOTIFY_H

#include <cstddef>

#include <boost/assert.hpp>
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
    std::size_t     use_count_;

protected:
    virtual void deallocate_object() = 0;

public:
    typedef intrusive_ptr< notify >     ptr_t;

    notify() :
        use_count_( 0)
    {}

    virtual ~notify() {};

    virtual bool is_ready() const BOOST_NOEXCEPT = 0;

    virtual void set_ready() BOOST_NOEXCEPT = 0;

    friend inline void intrusive_ptr_add_ref( notify * p) BOOST_NOEXCEPT
    { ++p->use_count_; }

    friend inline void intrusive_ptr_release( notify * p)
    { if ( --p->use_count_ == 0) p->deallocate_object(); }
};

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DETAIL_NOTIFY_H
