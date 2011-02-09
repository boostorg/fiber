/*
 * Boost.Reflection / inheritance tests
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
#include <iostream>

using namespace boost::reflections;

class A {
 public:
  virtual ~A() {}
  virtual char get_val() {
    return 'A';
  }
};

class B : public A {
 public:
  virtual char get_val() {
    return 'B';
  }
};

class C : virtual public A {
 public:
  virtual char get_val() {
    return 'C';
  }
};

class D : virtual public A {
 public:
  virtual char get_val() {
    return 'D';
  }
};

class E : public D, public C {
 public:
  virtual char get_val() {
    return 'E';
  }
};

class DSub : public D {
 public:
};

template <class T, char Name>
void TestClass() {
  reflection r;
  r.reflect<T>()
   .constructor()
   .function(&T::get_val, "get_val");

  instance i = r.get_constructor()();
  BOOST_CHECK_EQUAL(Name, r.get_function<char>("get_val")(i));

}

BOOST_AUTO_TEST_CASE(shared_library_basic_test) {
  TestClass<A, 'A'>();
  TestClass<B, 'B'>();
  TestClass<C, 'C'>();
  TestClass<D, 'D'>();
  TestClass<E, 'E'>();
  // TestClass<DSub, 'D'>();
}

BOOST_AUTO_TEST_CASE(DSubTest) {
  reflection r;
  r.reflect<DSub>()
   .constructor()
   .function(&DSub::get_val, "get_val");

  instance i = r.get_constructor()();
  BOOST_CHECK_EQUAL('D', r.get_function<char>("get_val")(i));
}