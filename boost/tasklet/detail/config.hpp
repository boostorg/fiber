
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// this file is based on config.hpp of boost.thread

#ifndef BOOST_TASKLETS_DETAIL_CONFIG_H
#define BOOST_TASKLETS_DETAIL_CONFIG_H

#include <boost/config.hpp>
#include <boost/detail/workaround.hpp>

#if defined(BOOST_ALL_DYN_LINK) || defined(BOOST_TASKLET_DYN_LINK)
# if defined(BOOST_TASKLET_SOURCE)
#  define BOOST_TASKLET_DECL BOOST_SYMBOL_EXPORT
# else 
#  define BOOST_TASKLET_DECL BOOST_SYMBOL_IMPORT
# endif
#else
# define BOOST_TASKLET_DECL
#endif

#if ! defined(BOOST_TASKLET_SOURCE) && !defined(BOOST_ALL_NO_LIB) && !defined(BOOST_TASKLET_NO_LIB)
# define BOOST_LIB_NAME boost_tasklet
# if defined(BOOST_ALL_DYN_LINK) || defined(BOOST_TASKLET_DYN_LINK)
#  define BOOST_DYN_LINK
# endif
# include <boost/config/auto_link.hpp>
#endif

#if defined(_MSC_VER)
# define BOOST_TASKLET_TSSDECL __declspec(thread)
#elif defined(__GNUC__)
# define BOOST_TASKLET_TSSDECL __thread
#else
# error "this platform is not supported"
#endif 

#endif // BOOST_TASKLETS_DETAIL_CONFIG_H
