
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef _BIND_PROCESSOR_H
#define _BIND_PROCESSOR_H

# if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#   include "bind_processor_windows.hpp"
# elif defined(linux) || defined(__linux) || defined(__linux__)
#   include "bind_processor_linux.hpp"
# elif defined(__IBMCPP__) || defined(_AIX)
#   include "bind_processor_aix.hpp"
# elif defined(__hpux)
#   include "bind_processor_hpux.hpp"
# elif defined(sun) || defined(__sun)
#   include "bind_processor_solaris.hpp"
# elif defined(__FreeBSD__)
#include <sys/param.h>
# if (__FreeBSD_version ">= 701000")
#   include "bind_processor_freebsd.hpp"
# endif
# error "platform not supported"
# endif

#endif // _BIND_PROCESSOR_H
