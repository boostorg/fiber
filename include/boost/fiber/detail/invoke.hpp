
//          Copyright Oliver Kowalke 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_DETAIL_INVOKE_H
#define BOOST_FIBERS_DETAIL_INVOKE_H

#include <functional>
#include <type_traits>
#include <utility>

#include <boost/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
# include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace detail {

template< typename Fn, typename... Args >
typename std::enable_if<
    std::is_member_pointer< typename std::decay< Fn >::type >::value,
    typename std::result_of< Fn( Args ...) >::type
>::type
invoke_( Fn && fn, Args && ... args) {
    return std::mem_fn( fn)( std::forward< Args >( args) ...);
}

template< typename Fn, typename ... Args >
typename std::enable_if<
    ! std::is_member_pointer< typename std::decay< Fn >::type >::value,
    typename std::result_of< Fn( Args ...) >::type
>::type
invoke_( Fn && fn, Args && ... args) {
    return fn( std::forward< Args >( args) ...);
}

template< typename Fn, typename Tpl, std::size_t... I >
decltype( auto) invoke_helper( Fn && fn, Tpl && tpl, std::index_sequence< I ... >) {
    return invoke_( std::forward< Fn >( fn),
                   // std::tuple_element<> does not perfect forwarding
                   std::forward< decltype( std::get< I >( std::declval< Tpl >() ) ) >(
                       std::get< I >( std::forward< Tpl >( tpl) ) ) ... );
}


template< typename Fn, typename Tpl >
decltype( auto) invoke_helper( Fn && fn, Tpl && tpl) {
    constexpr auto Size = std::tuple_size< typename std::decay< Tpl >::type >::value;
    return invoke_helper( std::forward< Fn >( fn),
                          std::forward< Tpl >( tpl),
                          std::make_index_sequence< Size >{});
}

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DETAIL_INVOKE_H
