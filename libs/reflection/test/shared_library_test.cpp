/*
 * Boost.Reflection / basic single parameter unit test
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
#include <boost/extension/factory_map.hpp>
#include <boost/extension/shared_library.hpp>
#include <boost/extension/convenience.hpp>
#include <boost/reflection/reflection.hpp>
#include <iostream>cd
#include <boost/function.hpp>

using namespace boost::reflections;
using namespace boost::extensions;

BOOST_AUTO_TEST_CASE(shared_library_basic_test) {
  std::map<std::string, reflection> reflection_map;
  shared_library lib
    ("libcar_lib.extension");
  BOOST_CHECK(lib.open());
  lib.get<void, std::map<std::string, reflection>&>
    ("extension_export_car")(reflection_map);
  BOOST_CHECK_EQUAL(reflection_map.size(), size_t(2));
  // Let's create the reflection and add the methods
  reflection & first_reflection =
    reflection_map["suv"];
  reflection & second_reflection =
    reflection_map["compact"];

  instance_constructor<const char *> first_constructor =
    first_reflection.get_constructor<const char *>();
  instance first_instance = 
    first_constructor("First Instance");
  function<const char *> first_function =
    first_reflection.get_function<const char *>("get_type");
  BOOST_CHECK_EQUAL(first_function(first_instance), "It's an SUV.");

  instance_constructor<const char *> second_constructor =
    second_reflection.get_constructor<const char *>();
  instance second_instance = 
    second_constructor("Second Instance");
  function<const char *> second_function =
    second_reflection.get_function<const char *>("get_type");
  BOOST_CHECK_EQUAL(second_function(second_instance), "It's a compact.");
}
