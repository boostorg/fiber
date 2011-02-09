/*
 * Boost.Reflection / Data Members example - Tutorial 1
 *
 * (C) Copyright Jeremy Pack 2008
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/ for latest version.
 */

#include <iostream>

#include <boost/reflection/reflection.hpp>

using boost::reflections::reflection;
using boost::reflections::instance;

void get_reflection(reflection& r);

int main(int argc, char ** argv) {
  reflection r = GetReflection();

  // Make sure this class supports a default constructor, and a function
  // identified as "print hello world" that has a void return value and
  // takes no arguments. If we wanted a constructor that took one int argument,
  // the call would become r.has_constructor<int>(). If we wanted a function
  // that took two float arguments, and returned a double, the call would be
  // r.has_function<void, float, float>("print hello world").
  if (r.get_constructor().valid() and
      r.get_function<void>("print hello world").valid()) {

    // Get and call the constructor to create an instance.
    instance i = r.get_constructor()();

    // Get and call the function called "print hello world".
    r.get_function<void>("print hello world")(i);
  } else {
    std::cerr << "Unable to find a required method.";
  }
}

struct data_holder {
  std::string my_string;
  int my_int;
};

void get_reflection(reflection& r) {
  r.reflect<data_holder>()
   .constructor()
   .data(&HelloWorld::my_int, "my integer")
   .data(&HelloWorld::my_string, "my string");
}