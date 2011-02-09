/*
 * Boost.Extension / main header:
 *         main header for extensions
 *
 * (C) Copyright Jeremy Pack 2007
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/ for latest version.
 */

#ifndef BOOST_EXTENSION_DECL_HPP
#define BOOST_EXTENSION_DECL_HPP

#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32) || defined(MSC_VER)
#  define BOOST_EXTENSION_EXPORT_DECL __declspec(dllexport)
#  define BOOST_EXTENSION_IMPORT_DECL __declspec(dllimport)
#else
#  define BOOST_EXTENSION_EXPORT_DECL
#  define BOOST_EXTENSION_IMPORT_DECL
#endif

#endif
