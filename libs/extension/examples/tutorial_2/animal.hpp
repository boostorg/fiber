/*
 * Boost.Extension / factory example (car)
 *
 * (C) Copyright Jeremy Pack 2007
 * Distributed under the Boost Software License, Version 1.0. (See             
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/ for latest version.
 */

#ifndef BOOST_EXTENSION_ANIMAL_HPP
#define BOOST_EXTENSION_ANIMAL_HPP

#include <iostream>
class animal {
public:
  animal(int age) : age_(age) {
  }
  virtual ~animal(void) {
  }
  virtual std::string get_name(void) {
    return "A generic animal";
  }
  int get_age(void) {
    return age_;
  }
protected:
  int age_;
};

#endif
