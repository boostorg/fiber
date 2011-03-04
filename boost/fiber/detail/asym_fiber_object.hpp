
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_DETAIL_ASYM_FIBER_OBJECT_H
#define BOOST_FIBERS_DETAIL_ASYM_FIBER_OBJECT_H

#include <cstddef>

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/move/move.hpp>
#include <boost/type_traits/remove_reference.hpp>

#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/asym_fiber_base.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace detail {

template< typename Fn >
class asym_fiber_object : public asym_fiber_base
{
private:
    Fn  fn_;

    asym_fiber_object( asym_fiber_object &);
    asym_fiber_object & operator=( asym_fiber_object const&);

public:
    asym_fiber_object( Fn fn, std::size_t stacksize, do_return_t v) :
        asym_fiber_base( stacksize, v),
        fn_( fn)
    {}

    asym_fiber_object( Fn fn, std::size_t stacksize, do_not_return_t v) :
        asym_fiber_base( stacksize, v),
        fn_( fn)
    {}

    void exec()
    { fn_(); }
};

template< typename Fn >
class asym_fiber_object< BOOST_RV_REF( Fn) > : public asym_fiber_base
{
private:
    Fn  fn_;

    asym_fiber_object( asym_fiber_object &);
    asym_fiber_object & operator=( asym_fiber_object const&);

public:
    asym_fiber_object( BOOST_RV_REF( Fn) fn, std::size_t stacksize, do_return_t v) :
        asym_fiber_base( stacksize, v),
        fn_( fn)
    {}

    asym_fiber_object( BOOST_RV_REF( Fn) fn, std::size_t stacksize, do_not_return_t v) :
        asym_fiber_base( stacksize, v),
        fn_( fn)
    {}

    void exec()
    { fn_(); }
};

template< typename Fn >
class asym_fiber_object< reference_wrapper< Fn > > : public asym_fiber_base
{
private:
    Fn  &   fn_;

    asym_fiber_object( asym_fiber_object &);
    asym_fiber_object & operator=( asym_fiber_object const&);

public:
    asym_fiber_object( reference_wrapper< Fn > fn, std::size_t stacksize, do_return_t v) :
        asym_fiber_base( stacksize, v),
        fn_( fn)
    {}

    asym_fiber_object( reference_wrapper< Fn > fn, std::size_t stacksize, do_not_return_t v) :
        asym_fiber_base( stacksize, v),
        fn_( fn)
    {}

    void exec()
    { fn_(); }
};

template< typename Fn >
class asym_fiber_object< const reference_wrapper< Fn > > : public asym_fiber_base
{
private:
    Fn  &   fn_;

    asym_fiber_object( asym_fiber_object &);
    asym_fiber_object & operator=( asym_fiber_object const&);

public:
    asym_fiber_object( const reference_wrapper< Fn > fn, std::size_t stacksize, do_return_t v) :
        asym_fiber_base( stacksize, v),
        fn_( fn)
    {}

    void exec()
    { fn_(); }
};

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DETAIL_ASYM_FIBER_OBJECT_H
