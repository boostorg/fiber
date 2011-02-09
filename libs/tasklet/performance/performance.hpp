
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef PERFORMANCE_H
#define PERFORMANCE_H

// msvc/icc, i386
#if defined(_MSC_VER) && defined(_M_IX86)
#include "performance_msvc_i386.hpp"

// gcc/icc, i386
#elif defined(__GNUC__) && defined(__i386__)
#include "performance_gcc_i386.hpp"

// gcc/icc, x86_64
#elif defined(__GNUC__) && defined(__x86_64__)
#include "performance_gcc_x86-64.hpp"

#else
#error "this platform is not supported"
#endif 

#endif // PERFORMANCE_H
