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

#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK 1
#include <boost/test/unit_test.hpp>

#include <iostream>

#include <boost/reflection/reflection.hpp>
using namespace boost::reflections;

struct data_holder {
  std::string my_string;
  int my_int;
  std::string double_string() {
    return my_string + my_string;
  }
};

using namespace boost::reflections;

BOOST_AUTO_TEST_CASE(simple)
{
  reflection r;
  r.reflect<data_holder>()
   .constructor()
   .function(&data_holder::double_string, "double_string")
   .data(&data_holder::my_string, "my string")
   .data(&data_holder::my_int, "my integer");

  instance i = r.get_constructor()();
  data<std::string> d = r.get_data<std::string>("my string");
  BOOST_CHECK(d.valid());
  std::string& s = d(i);
  BOOST_CHECK(s.empty());
  s = "Hello!";
  std::string result = r.get_function<std::string>("double_string")(i);
  BOOST_CHECK_EQUAL(result,
                    std::string("Hello!Hello!"));
}