/*
 * Boost.Extension / testing of type_map class
 *
 * (C) Copyright Jeremy Pack 2008
 * Distributed under the Boost Software License, Version 1.0. (See             
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/ for latest version.
 */

#include <boost/extension/type_map.hpp>
#include <boost/function.hpp>
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK 1
#include <boost/test/unit_test.hpp>


using namespace boost::extensions;

BOOST_AUTO_TEST_CASE(intTest) {
  type_map m;
  int& my_int(m.get());
  my_int = 5;
  int& my_second_int(m.get());
  BOOST_CHECK_EQUAL(my_second_int, 5);
}

BOOST_AUTO_TEST_CASE(intFloatTest) {
  type_map m;
  int& my_int(m.get());
  my_int = 5;
  float& my_float(m.get());
  my_float = 10.0;
  BOOST_CHECK_EQUAL(my_int, 5);
  BOOST_CHECK_EQUAL(my_float, 10.0);
}

BOOST_AUTO_TEST_CASE(constTest) {
  type_map m;
  int& my_int(m.get());
  my_int = 5;
  const int& my_const_int(m.get());
  BOOST_CHECK_EQUAL(my_const_int, 5);
}
