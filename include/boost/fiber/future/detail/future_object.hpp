
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_DETAIL_FUTURE_OBJECT_H
#define BOOST_FIBERS_DETAIL_FUTURE_OBJECT_H

#include <boost/config.hpp>

#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/future/detail/future_base.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace detail {

template< typename R, typename Allocator >
class future_object : public future_base< R >
{
public:
    typedef typename Allocator::template rebind<
        future_object< R, Allocator >
    >::other                                      allocator_t;

    future_object( allocator_t const& alloc) :
        future_base< R >(), alloc_( alloc)
    {}

protected:
    void deallocate_future()
    { destroy_( alloc_, this); }

private:
    allocator_t             alloc_;

    static void destroy_( allocator_t & alloc, future_object * p)
    {
        alloc.destroy( p);
        alloc.deallocate( p, 1);
    }
};

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DETAIL_FUTURE_BASE_H
