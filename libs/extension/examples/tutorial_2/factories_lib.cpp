/*
 * Boost.Extension / hello world implementations
 *
 * (C) Copyright Jeremy Pack 2007-2008
 * Distributed under the Boost Software License, Version 1.0. (See             
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/ for latest version.
 */

#include <iostream>
#include <map>

#include <boost/extension/extension.hpp>
#include <boost/extension/factory.hpp>
#include <boost/extension/type_map.hpp>

#include "animal.hpp"

class puma : public animal {
 public:
  puma(int age) : animal(age) {}
  virtual std::string get_name() {
    return "puma";
  }
};

class leopard : public animal {
 public:
  leopard(int age) : animal(age) {}
  virtual std::string get_name() {
    return "leopard";
  }
};

class wildcat : public animal {
 public:
  wildcat(int age) : animal(age) {}
  virtual std::string get_name() {
    return "wildcat";
  }
};

class cougar : public animal {
 public:
  cougar(int age) : animal(age) {}
  virtual std::string get_name() {
    return "cougar";
  }
};


BOOST_EXTENSION_TYPE_MAP_FUNCTION {
  using namespace boost::extensions;
  std::map<std::string, factory<animal, int> >&
    animal_factories(types.get());
  animal_factories["Puma factory"].set<puma>();
  animal_factories["Leopard factory"].set<leopard>();
  animal_factories["Wildcat factory"].set<wildcat>();
  animal_factories["Cougar factory"].set<cougar>();
}
