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
#include <boost/reflection/reflection.hpp>

class car {
public:
  car(float cost) : cost_(cost) {}
  float get_cost() {return cost_;}
  int start(bool fast) {
    if (fast)
      return 60;
    else
      return 30;
  }
private:
  float cost_;
};
using namespace boost::reflections;
BOOST_AUTO_TEST_CASE(argless)
{
  reflection car_reflection;
  car_reflection.reflect<car>()
                .constructor<float>()
                .function<int, bool>(&car::start, "start")
                .function<float>(&car::get_cost, "get_cost");
  //  Check for argless constructor
  BOOST_CHECK(car_reflection.get_constructor<float>().valid());
  instance car_instance = 
    car_reflection.get_constructor<float>().call(10.0f);
  function<int, bool> f1 = 
    car_reflection.get_function<int, bool>("start");
  BOOST_CHECK(f1.valid());
  int ret_val = f1(car_instance, false);
  BOOST_CHECK_EQUAL
    (ret_val, 30);
  BOOST_CHECK_EQUAL
    (car_reflection.get_function<float>("get_cost")
     .call(car_instance), 10.0f);
  function<float> f2 =
    car_reflection.get_function<float>("get_cost");
  BOOST_CHECK_EQUAL(f2(car_instance), 10.0f);
}
