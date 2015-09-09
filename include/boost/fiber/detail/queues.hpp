
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

struct state_tag;
typedef intrusive::list_base_hook<
    intrusive::tag< state_tag >,
    intrusive::link_mode<
        intrusive::safe_link
    >
>                                       state_hook;
template< typename T >
using state_queue = intrusive::list<
                        T,
                        intrusive::base_hook< state_hook > >;

struct yield_tag;
typedef intrusive::list_base_hook<
    intrusive::tag< yield_tag >,
    intrusive::link_mode<
        intrusive::safe_link
    >
>                                       yield_hook;
template< typename T >
using yield_queue = intrusive::list<
                        T,
                        intrusive::base_hook< yield_hook >,
                        intrusive::constant_time_size< false > >;

struct wait_tag;
typedef intrusive::list_base_hook<
    intrusive::tag< wait_tag >,
    intrusive::link_mode<
        intrusive::auto_unlink
    >
>                                       wait_hook;
template< typename T >
using wait_queue = intrusive::list<
                        T,
                        intrusive::base_hook< wait_hook >,
                        intrusive::constant_time_size< false > >;

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DETAIL_QUEUES_H
