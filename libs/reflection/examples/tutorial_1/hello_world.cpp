/*
 * Boost.Reflection / Hello World example - Tutorial 1
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

reflection GetReflection();

int main(int argc, char ** argv) {
  reflection r = GetReflection();

  // Make sure this class supports a default constructor, and a function
  // identified as "print hello world" that has a void return value and
  // takes no arguments. If we wanted a constructor that took one int argument,
  // the call would become r.has_constructor<int>(). If we wanted a function
  // that took two float arguments, and returned a double, the call would be
  // r.has_function<void, float, float>("print hello world").
  if (r.get_constructor().valid() &&
      r.get_function<void>("print hello world").valid()) {

    // Get and call the constructor to create an instance.
    instance i = r.get_constructor()();

    // Get and call the function called "print hello world".
    r.get_function<void>("print hello world")(i);
  } else {
    std::cerr << "Unable to find a required method.";
  }
}

class HelloWorld {
public:
  void printHelloWorld() {
    std::cout << "Hello World!" << std::endl;
  }
};

reflection GetReflection() {
  // This reflection will be returned
  reflection r;
  r.reflect<HelloWorld>()
   .constructor()
   .function(&HelloWorld::printHelloWorld,
             "print hello world");
  return r;
}