/*
 * Boost.Reflection / parameter map unit test
 *
 * (C) Copyright Jeremy Pack 2008
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/ for latest version.
 */


#include <string>
#include <iostream>

#define BOOST_EXTENSION_USE_PP 1

#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK 1
#include <boost/test/unit_test.hpp>
#include <boost/reflection/reflection.hpp>
#include <boost/reflection/parameter_map.hpp>
using namespace boost::reflections;
BOOST_AUTO_TEST_CASE(paramter_map) {
  parameter_map<std::string,
                boost::extension::default_type_info> pm;
  int m = 5;
  int j = 6;
  parameter<int &> it =
    pm.insert<int &>(m, "integer m");
  it->converts_to<float>();
  it->converts_to<short>();
  it->converts_to<long>();
  it->converts_to<unsigned long>();
  it = pm.insert<int &>(j, "integer m");
  it->converts_to<float>();
  BOOST_CHECK_EQUAL(pm.has<float>());
  float val = pm.get<float>("integer j");
  BOOST_CHECK_EQUAL(val, 5.0f);
  float val2 = pm.get<float>("integer j");
  BOOST_CHECK_EQUAL(val2, 6.0f);
}
