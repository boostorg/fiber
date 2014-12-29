
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_DETAIL_WORKER_FIBER_H
#define BOOST_FIBERS_DETAIL_WORKER_FIBER_H

#include <memory> // std::allocator

#include <boost/config.hpp>

#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/fiber_base.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace detail {

template< typename Allocator >
class worker_fiber : public fiber_base {
public:
    typedef typename std::allocator_traits< Allocator >::template rebind_alloc< worker_fiber >  allocator_type;

    template< typename StackAlloc, typename Fn >
    explicit worker_fiber( Allocator alloc, StackAlloc salloc, Fn && fn) :
        fiber_base( salloc, std::forward< Fn >( fn) ),
        alloc_( alloc) {
    }

    ~worker_fiber() {
    }

protected:
    void deallocate() override final {
        allocator_type alloc( alloc_);
        worker_fiber * p = this;
        std::allocator_traits< allocator_type >::destroy( alloc, p);
        std::allocator_traits< allocator_type >::deallocate( alloc, p, 1);
    }

private:
    allocator_type  alloc_;
};

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DETAIL_WORKER_FIBER_H
