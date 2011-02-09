
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TASKS_DETAIL_BIND_PROCESSOR_H
#define BOOST_TASKS_DETAIL_BIND_PROCESSOR_H

#include <boost/task/detail/config.hpp>

# if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#   define BOOST_HAS_PROCESSOR_BINDINGS 1
#   include <boost/task/detail/bind_processor_windows.hpp>
# elif defined(linux) || defined(__linux) || defined(__linux__)
#   define BOOST_HAS_PROCESSOR_BINDINGS 1
#   include <boost/task/detail/bind_processor_linux.hpp>
# elif defined(__IBMCPP__) || defined(_AIX)
#   define BOOST_HAS_PROCESSOR_BINDINGS 1
#   include <boost/task/detail/bind_processor_aix.hpp>
# elif defined(__hpux)
#   define BOOST_HAS_PROCESSOR_BINDINGS 1
#   include <boost/task/detail/bind_processor_hpux.hpp>
# elif defined(sun) || defined(__sun)
#   define BOOST_HAS_PROCESSOR_BINDINGS 1
#   include <boost/task/detail/bind_processor_solaris.hpp>
# elif defined(__FreeBSD__)
#include <sys/param.h>
# if (__FreeBSD_version >= 701000)
#   define BOOST_HAS_PROCESSOR_BINDINGS 1
#   include <boost/task/detail/bind_processor_freebsd.hpp>
# endif
# else
#   undef BOOST_HAS_PROCESSOR_BINDINGS
# endif

#endif // BOOST_TASKS_DETAIL_BIND_PROCESSOR_H
