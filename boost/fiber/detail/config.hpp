
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// this file is based on config.hpp of boost.thread

#ifndef BOOST_FIBERS_DETAIL_CONFIG_H
#define BOOST_FIBERS_DETAIL_CONFIG_H

#include <boost/config.hpp>
#include <boost/detail/workaround.hpp>

#if defined(BOOST_ALL_DYN_LINK) || defined(BOOST_FIBER_DYN_LINK)
# if defined(BOOST_FIBER_SOURCE)
#  define BOOST_FIBER_DECL BOOST_SYMBOL_EXPORT
# else 
#  define BOOST_FIBER_DECL BOOST_SYMBOL_IMPORT
# endif
#else
# define BOOST_FIBER_DECL
#endif

#if ! defined(BOOST_FIBER_SOURCE) && !defined(BOOST_ALL_NO_LIB) && !defined(BOOST_FIBER_NO_LIB)
# define BOOST_LIB_NAME boost_fiber
# if defined(BOOST_ALL_DYN_LINK) || defined(BOOST_FIBER_DYN_LINK)
#  define BOOST_DYN_LINK
# endif
# include <boost/config/auto_link.hpp>
#endif

#endif // BOOST_FIBERS_DETAIL_CONFIG_H
