
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_THREAD_LOCAL_PTR_H
#define BOOST_FIBERS_THREAD_LOCAL_PTR_H

#include <boost/config.hpp>
#include <boost/utility.hpp>
#include <boost/thread/tss.hpp>

#include <boost/fiber/detail/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

template< typename T >
struct thread_local_ptr : public boost::thread_specific_ptr< T >
{};

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_THREAD_LOCAL_PTR_H
