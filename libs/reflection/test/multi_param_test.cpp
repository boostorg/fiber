/*
 * Boost.Reflection / basic many parameter unit test
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
  car(float cost, const char * color, int quantity) : cost_(cost) {}
  float get_cost(const char * color) {return cost_;}
  int start(bool fast, float direction) {
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
                .constructor<float, const char *, int>()
                .function<int, bool, float>(&car::start, "start")
                .function(&car::get_cost, "get_cost");
  //  Check for argless constructor
  BOOST_CHECK((car_reflection.get_constructor<float, const char *, int>
              ().valid()));
  instance car_instance = 
    car_reflection.get_constructor<float, const char *, int>()
    .call(10.0f, "red", 2);
  function<int, bool, float> f1 =
    car_reflection.get_function<int, bool, float>("start");
  BOOST_CHECK(f1.valid());
  int ret_val = f1(car_instance, false, 90.0f);
  BOOST_CHECK_EQUAL
    (ret_val, 30);
  BOOST_CHECK_EQUAL
    ((car_reflection.get_function<float, const char *>("get_cost")
     .call(car_instance, "blue")), 10.0f);
  function<float, const char *> f2 =
    car_reflection.get_function<float, const char *>("get_cost");
  BOOST_CHECK_EQUAL((f2(car_instance, "green")), 10.0f);
}
