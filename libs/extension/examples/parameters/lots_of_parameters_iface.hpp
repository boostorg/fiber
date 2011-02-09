/*
 * Boost.Extension / lots of parameters interface
 *
 * (C) Copyright Mariano G. Consoni 2007
 * Distributed under the Boost Software License, Version 1.0. (See             
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/ for latest version.
 */

#include <boost/shared_ptr.hpp>

#include <string>

class A
{
public:
  A(unsigned int i) : i(i) {}
  int i;
};


class lots_of_parameters_interface
{
public:
  lots_of_parameters_interface(bool b, unsigned int i, char c, std::string s, 
                               A a, boost::shared_ptr<A> ptr_a) {}
  virtual ~lots_of_parameters_interface(void) {}
};

