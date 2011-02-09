/*
 * Boost.Extension / registry unit test
 *
 * (C) Copyright Jeremy Pack 2007
 * Distributed under the Boost Software License, Version 1.0. (See             
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/ for latest version.
 */

#include <string>
#include <boost/extension/registry.hpp>

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

#include "../examples/word.hpp"

using namespace boost::extensions;

BOOST_AUTO_TEST_CASE(registry_construction)
{
  registry reg;
  BOOST_CHECK_EQUAL(reg.num_libraries(), size_t(0));
  BOOST_CHECK(reg.open("libRegistryLibrary.extension"));
  BOOST_CHECK_EQUAL(reg.num_libraries(), size_t(1));
  BOOST_CHECK(reg.close("libRegistryLibrary.extension"));
  BOOST_CHECK_EQUAL(reg.num_libraries(), size_t(0));
}

BOOST_AUTO_TEST_CASE(library_closing)
{  // this tests the requirements of the registry example
  registry reg;
  BOOST_ASSERT(reg.open("libRegistryLibrary.extension"));
  {
    std::list<counted_factory<word, int> > & factory_list = 
      reg.get<word, int>(); 
    BOOST_CHECK_EQUAL(factory_list.size(), size_t(1));
    {
      std::auto_ptr<word> w(factory_list.begin()->create());
      BOOST_CHECK_EQUAL(std::string(w->get_val()), std::string("First Time"));
      BOOST_CHECK_EQUAL(std::string(w->get_val()), std::string("Second Time"));
    }
  }
  BOOST_CHECK(reg.clear());
  BOOST_ASSERT(reg.open("libRegistryLibrary.extension"));
  {
    std::list<counted_factory<word, int> > & factory_list = 
      reg.get<word, int>(); 
    BOOST_CHECK_EQUAL(factory_list.size(), size_t(1));
    {
      std::auto_ptr<word> w(factory_list.begin()->create());
      BOOST_CHECK_EQUAL(std::string(w->get_val()), std::string("First Time"));
      BOOST_CHECK_EQUAL(std::string(w->get_val()), std::string("Second Time"));
    }
  }
  BOOST_CHECK(reg.clear());
  BOOST_CHECK_EQUAL(reg.num_libraries(), size_t(0));
}
