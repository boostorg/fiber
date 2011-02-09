/*
 * Boost.Reflection / basic unit test
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
#include <boost/reflection/reflection.hpp>

class car {
public:
  int start() {
    return 3;
  }
};

using namespace boost::reflections;
BOOST_AUTO_TEST_CASE(argless) {
  basic_reflection<std::string, std::string> car_reflection;
  car_reflection.reflect<car>()
                .constructor()
                .function<int>(&car::start, "start", "speed");

  //  Check for argless constructor
  BOOST_CHECK(car_reflection.get_constructor().valid());
  instance car_instance =
    car_reflection.get_constructor().call();
  BOOST_CHECK(car_reflection.get_function<int>("start").valid());
  //  Make sure it doesn't have this undeclared method
  BOOST_CHECK(!car_reflection.get_function<int>("stop").valid());
  BOOST_CHECK_EQUAL
    (car_reflection.get_function<int>("start").call(car_instance), 3);
  function<int> f =
    car_reflection.get_function<int>("start");
  BOOST_CHECK_EQUAL(f(car_instance), 3);
}