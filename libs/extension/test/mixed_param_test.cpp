/*
 * Boost.Extension / construction test case
 *
 * (C) Copyright Jeremy Pack 2007
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/ for latest version.
 */


#include <boost/extension/factory.hpp>
#include <boost/extension/factory_map.hpp>
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>
#include <memory>
#include <map>

using namespace boost::extensions;


class Base {
public:
  Base(int i) : value_(i) {}
  Base(float j) : value_(int(j)) {}
  Base() : value_(23) {}
  Base(int i, float j) : value_(int(j) * i) {}
  virtual int getValue() {return value_;}
  virtual ~Base() {}
private:
  int value_;
};
class Derived : public Base {
public:
  virtual ~Derived() {}
  Derived(int i) : Base(float(i * 3)) {}
  Derived(float j) : Base(int(j)) {}
  Derived() : Base(2, 23.0f) {}
  Derived(int i, float j) : Base() {}
  virtual int getValue() {return 2 * Base::getValue();}
};




BOOST_AUTO_TEST_CASE(CorrectConstructor) {
  factory_map m;
  m.get<Base, std::string, int, float>()["Derived"].set<Derived>();
  m.get<Base, std::string, int>()["Derived"].set<Derived>();
  m.get<Base, std::string>()["Derived"].set<Derived>();
  m.get<Base, std::string, float>()["Derived"].set<Derived>();
  m.get<Base, std::string, int, float>()["Base"].set<Base>();
  m.get<Base, std::string, int>()["Base"].set<Base>();
  m.get<Base, std::string>()["Base"].set<Base>();
  m.get<Base, std::string, float>()["Base"].set<Base>();
  std::auto_ptr<Base> b1(m.get<Base, std::string, float>()
                         ["Derived"].create(2.0f));
  BOOST_CHECK_EQUAL(b1->getValue(), 4);
  std::auto_ptr<Base> b2(m.get<Base, std::string>()
                         ["Derived"].create());
  BOOST_CHECK_EQUAL(b2->getValue(), 92);
  std::auto_ptr<Base> b3(m.get<Base, std::string, int, float>()
                         ["Base"].create(4, 5.0f));
  BOOST_CHECK_EQUAL(b3->getValue(), 20.0f);
}