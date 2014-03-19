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
    static main_fiber * make_pointer( main_fiber & n) {
        return & n;
    }

    main_fiber() :
        fiber_base(),
        ready_( false)
    {}

    bool is_ready() const BOOST_NOEXCEPT
    { return ready_; }

    void set_ready() BOOST_NOEXCEPT
    { ready_ = true; }

    id get_id() const BOOST_NOEXCEPT
    { return id( const_cast< main_fiber * >( this) ); }

private:
    atomic< bool >  ready_;

    main_fiber( main_fiber const&);
    main_fiber & operator=( main_fiber const&);
};

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DETAIL_MAIN_FIBER_H
