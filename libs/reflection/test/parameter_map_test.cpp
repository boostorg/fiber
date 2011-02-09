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


#include <cmath>
#include <iostream>
#include <string>

#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK 1
#include <boost/test/unit_test.hpp>
#include <boost/reflection/parameter_map.hpp>

using boost::reflections::parameter_map;
using boost::reflections::generic_parameter;
using boost::reflections::parameter;
BOOST_AUTO_TEST_CASE(construction) {
  parameter<int>* p = new parameter<int>(5);
  parameter_map m;
  m.insert(std::make_pair("some_integer", p));
}

// Verify that the parameter map deletes it's elements
// when it is done with them.
class counted_creation {
public:
  counted_creation() {
    ++total_objects;
  }
  counted_creation(const counted_creation& left) {
    ++total_objects;
  }
  ~counted_creation() {
    --total_objects;
  }
  static int total_objects;
};
int counted_creation::total_objects = 0;
BOOST_AUTO_TEST_CASE(destroy_elements) {
  {
    parameter<counted_creation>* p =
      new parameter<counted_creation>(counted_creation());
    BOOST_CHECK_EQUAL(counted_creation::total_objects, 1);
    parameter_map m;
    m.insert(std::make_pair("some_integer", p));
    BOOST_CHECK_EQUAL(counted_creation::total_objects, 1);
  }
  BOOST_CHECK_EQUAL(counted_creation::total_objects, 0);
}

BOOST_AUTO_TEST_CASE(float_convert) {
  parameter<int>* p = new parameter<int>(5);
  p->converts_to<float>();
  p->converts_to<double>();
  generic_parameter<>* g = p;

  BOOST_CHECK(g->can_cast<float>());
  BOOST_CHECK(g->can_cast<double>());
  BOOST_CHECK(!g->can_cast<char>());
  BOOST_CHECK(!g->can_cast<short>());

  BOOST_CHECK_EQUAL(5.0f, g->cast<float>());
  BOOST_CHECK_EQUAL(5.0, g->cast<double>());
  BOOST_CHECK_EQUAL(5, g->cast<int>());
  parameter_map m;
  m.insert(std::make_pair("some_integer", p));
  m.insert(std::make_pair("some_other_integer",
                          new parameter<int>(12)));
  g = m.get_first<int>("some_integer");

  BOOST_CHECK(g->can_cast<float>());
  BOOST_CHECK(g->can_cast<double>());
  BOOST_CHECK(!g->can_cast<char>());
  BOOST_CHECK(!g->can_cast<short>());

  BOOST_CHECK_EQUAL(5.0f, g->cast<float>());
  BOOST_CHECK_EQUAL(5.0, g->cast<double>());
  BOOST_CHECK_EQUAL(5, g->cast<int>());

  g = m.get_first<int>("some_other_integer");

  BOOST_CHECK(g->can_cast<int>());

  BOOST_CHECK_EQUAL(12, g->cast<int>());
}

void FloatCeilingToInt(float* f, int* i) {
  *i = static_cast<int>(std::ceil(*f));
}

BOOST_AUTO_TEST_CASE(converts_to_with_func) {
  parameter<float>* p = new parameter<float>(4.9f);
  p->converts_to_with_func(&FloatCeilingToInt);
  BOOST_CHECK(p->can_cast<int>());
  BOOST_CHECK_EQUAL(p->cast<int>(), 5);
}

class base {
};
class derived : public base {
};

void silly_convert(derived** b, float* f) {
  *f = 86;
}

BOOST_AUTO_TEST_CASE(ptr_convert) {
  derived d;
  parameter<derived*>* p = new parameter<derived*>(&d);
  p->converts_to<base*>();
  p->converts_to_with_func<float>(&silly_convert);
  generic_parameter<>* g = p;

  BOOST_CHECK(g->can_cast<float>());
  BOOST_CHECK(g->can_cast<base*>());
  BOOST_CHECK(g->can_cast<derived*>());
  BOOST_CHECK(!g->can_cast<double>());

  BOOST_CHECK_EQUAL(&d, g->cast<derived*>());
  BOOST_CHECK_EQUAL(86, g->cast<float>());
}