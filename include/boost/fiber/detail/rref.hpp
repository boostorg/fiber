
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_DETAIL_RREF_H
#define BOOST_FIBERS_DETAIL_RREF_H

#include <utility>

#include <boost/config.hpp>

#include <boost/coroutine/detail/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace detail {

// helper class for capture move-only objects
// generalized lambda captures are supported by C++14
template< typename Fn >
class rref {
public:
    rref( Fn && fn) :
        fn_( std::move( fn) ) {
    }

    rref( rref & other) :
        fn_( std::move( other.fn_) ) {
    }

    rref( rref && other) :
        fn_( std::move( other.fn_) ) {
    }

    rref( rref const& other) = delete;
    rref & operator=( rref const& other) = delete;

    void operator()() {
        return fn_();
    }

private:
    Fn  fn_;
};

template< typename T >
rref< T > make_rref( T && t) {
    return rref< T >( std::move( t) );
}

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DETAIL_RREF_H
