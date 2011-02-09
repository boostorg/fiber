/*
 * Boost.Extension / hello world implementations
 *
 * (C) Copyright Jeremy Pack 2007-2008
 * Distributed under the Boost Software License, Version 1.0. (See             
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/ for latest version.
 */

#include <iostream>

#include <boost/extension/extension.hpp>

// Any exported function or variable must be declared
// extern "C" to avoid C++ name mangling.
extern "C"
// Depending on the compiler and settings,
// it may be necessary to add a specific export
// declaration. The BOOST_EXTENSION_EXPORT_DECL
// adds this if necessary.
void BOOST_EXTENSION_EXPORT_DECL
boost_extension_hello_world(int repetitions) {
  for (int i = 0; i < repetitions; ++i) {
    std::cout << "Hello World" << std::endl;
  }
}
