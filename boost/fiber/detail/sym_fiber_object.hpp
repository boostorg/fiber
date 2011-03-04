
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software Licenseersion 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_DETAIL_SYM_FIBER_OBJECT_H
#define BOOST_FIBERS_DETAIL_SYM_FIBER_OBJECT_H

#include <cstddef>

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/move/move.hpp>
#include <boost/type_traits/remove_reference.hpp>

#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/sym_fiber_base.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace detail {

class sym_fiber_dummy : public sym_fiber_base
{
private:
    sym_fiber_dummy( sym_fiber_dummy &);
    sym_fiber_dummy & operator=( sym_fiber_dummy const&);

public:
    sym_fiber_dummy() :
        sym_fiber_base()
    {}

    void exec()
    { BOOST_ASSERT( false && "should not be invoked"); }
};

template< typename Fn >
class sym_fiber_object : public sym_fiber_base
{
private:
    Fn  fn_;

    sym_fiber_object( sym_fiber_object &);
    sym_fiber_object & operator=( sym_fiber_object const&);

public:
    sym_fiber_object( Fn fn, std::size_t stacksize) :
        sym_fiber_base( stacksize),
        fn_( fn)
    {}

    sym_fiber_object( Fn fn, std::size_t stacksize, sym_fiber_base & nxt) :
        sym_fiber_base( stacksize, nxt),
        fn_( fn)
    {}

    void exec()
    { fn_(); }
};

template< typename Fn >
class sym_fiber_object< BOOST_RV_REF( Fn) > : public sym_fiber_base
{
private:
    Fn  fn_;

    sym_fiber_object( sym_fiber_object &);
    sym_fiber_object & operator=( sym_fiber_object const&);

public:
    sym_fiber_object( BOOST_RV_REF( Fn) fn, std::size_t stacksize) :
        sym_fiber_base( stacksize),
        fn_( fn)
    {}

    sym_fiber_object( BOOST_RV_REF( Fn) fn, std::size_t stacksize, sym_fiber_base & nxt) :
        sym_fiber_base( stacksize, nxt),
        fn_( fn)
    {}

    void exec()
    { fn_(); }
};

template< typename Fn >
class sym_fiber_object< reference_wrapper< Fn > > : public sym_fiber_base
{
private:
    Fn  &   fn_;

    sym_fiber_object( sym_fiber_object &);
    sym_fiber_object & operator=( sym_fiber_object const&);

public:
    sym_fiber_object( reference_wrapper< Fn > fn, std::size_t stacksize) :
        sym_fiber_base( stacksize),
        fn_( fn)
    {}

    sym_fiber_object( reference_wrapper< Fn > fn, std::size_t stacksize, sym_fiber_base & nxt) :
        sym_fiber_base( stacksize, nxt),
        fn_( fn)
    {}

    void exec()
    { fn_(); }
};

template< typename Fn >
class sym_fiber_object< const reference_wrapper< Fn > > : public sym_fiber_base
{
private:
    Fn  &   fn_;

    sym_fiber_object( sym_fiber_object &);
    sym_fiber_object & operator=( sym_fiber_object const&);

public:
    sym_fiber_object( const reference_wrapper< Fn > fn, std::size_t stacksize) :
        sym_fiber_base( stacksize),
        fn_( fn)
    {}

    sym_fiber_object( const reference_wrapper< Fn > fn, std::size_t stacksize, sym_fiber_base & nxt) :
        sym_fiber_base( stacksize, nxt),
        fn_( fn)
    {}

    void exec()
    { fn_(); }
};

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DETAIL_SYM_FIBER_OBJECT_H
