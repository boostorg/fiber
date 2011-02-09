/*
 * Boost.Extension / executables unit test
 *
 * (C) Copyright Jeremy Pack 2008
 * Distributed under the Boost Software License, Version 1.0. (See             
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/ for latest version.
 */

#include <cstdlib>

#include <iostream>

#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK 1
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(multiple_inheritance_example) {
  BOOST_CHECK_EQUAL(std::system("./MultipleInheritance"), 0);
}
