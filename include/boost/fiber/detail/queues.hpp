
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_DETAIL_QUEUES_H
#define BOOST_FIBERS_DETAIL_QUEUES_H

#include <boost/config.hpp>
#include <boost/intrusive/list.hpp>

#include <boost/fiber/detail/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace detail {

struct runnable_tag;
typedef intrusive::list_member_hook<
    intrusive::tag< runnable_tag >,
    intrusive::link_mode<
        intrusive::safe_link
    >
>                                       runnable_hook;
template< typename T >
using runnable_queue = intrusive::list<
                        T,
                        intrusive::member_hook< T, runnable_hook, & T::runnable_hook_ > >;

struct ready_tag;
typedef intrusive::list_member_hook<
    intrusive::tag< ready_tag >,
    intrusive::link_mode<
        intrusive::safe_link
    >
>                                       ready_hook;
template< typename T >
using ready_queue = intrusive::list<
                        T,
                        intrusive::member_hook< T, ready_hook, & T::ready_hook_ >,
                        intrusive::constant_time_size< false > >;

struct sleep_tag;
typedef intrusive::list_member_hook<
    intrusive::tag< sleep_tag >,
    intrusive::link_mode<
        intrusive::auto_unlink
    >
>                                       sleep_hook;
template< typename T >
using sleep_queue = intrusive::list<
                        T,
                        intrusive::member_hook< T, sleep_hook, & T::sleep_hook_ >,
                        intrusive::constant_time_size< false > >;

struct wait_tag;
typedef intrusive::list_member_hook<
    intrusive::tag< wait_tag >,
    intrusive::link_mode<
        intrusive::auto_unlink
    >
>                                       wait_hook;
template< typename T >
using wait_queue = intrusive::list<
                        T,
                        intrusive::member_hook< T, wait_hook, & T::wait_hook_ >,
                        intrusive::constant_time_size< false > >;

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DETAIL_QUEUES_H
