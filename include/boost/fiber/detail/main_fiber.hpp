//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_DETAIL_MAIN_FIBER_H
#define BOOST_FIBERS_DETAIL_MAIN_FIBER_H

#include <boost/atomic.hpp>
#include <boost/config.hpp>

#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/fiber_base.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace detail {

class main_fiber : public fiber_base
{
public:
    main_fiber() :
        fiber_base( 0) // main-fiber represents main-context
    {
        thread_affinity( true);
        set_running();
    }

    void deallocate() {}
};

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DETAIL_MAIN_FIBER_H
