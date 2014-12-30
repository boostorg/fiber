
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_DETAIL_RREF_H
#define BOOST_FIBERS_DETAIL_RREF_H

#include <algorithm>
#include <utility>

#include <boost/config.hpp>

#include <boost/coroutine/detail/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace detail {

// helper classes for capture move-only objects
// generalized lambda captures are supported by C++14
template< typename T >
class arg_rref {
public:
    arg_rref( T && t) :
        t_( std::forward< T >( t) ) {
    }

    arg_rref( arg_rref & other) :
        t_( std::forward< T >( other.t_) ) {
    }

    arg_rref( arg_rref && other) :
        t_( std::forward< T >( other.t_) ) {
    }

    arg_rref( arg_rref const& other) = delete;
    arg_rref & operator=( arg_rref const& other) = delete;

    T && move() {
        return std::move( t_);
    }

private:
    T   t_;
};

template< typename Fn >
class fn_rref {
public:
    fn_rref( Fn && fn) :
        fn_( std::forward< Fn >( fn) ) {
    }

    fn_rref( fn_rref & other) :
        fn_( std::forward< Fn >( other.fn_) ) {
    }

    fn_rref( fn_rref && other) :
        fn_( std::forward< Fn >( other.fn_) ) {
    }

    fn_rref( fn_rref const& other) = delete;
    fn_rref & operator=( fn_rref const& other) = delete;

    template< typename ... Args >
    void operator()( arg_rref< Args > ... arg_rr) {
        return fn_( arg_rr.move() ... );
    }

private:
    Fn  fn_;
};

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DETAIL_RREF_H
