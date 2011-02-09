/*
 * Boost.Extension / factory unit test
 *
 * (C) Copyright Jeremy Pack 2007
 * Distributed under the Boost Software License, Version 1.0. (See             
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/ for latest version.
 */

#include <boost/extension/factory_map.hpp>
#include <boost/extension/factory.hpp>
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

#include "fruit.hpp"
#include <vector>
#include <list>
using namespace boost::extensions;
void setup_zone(factory_map & z)
{
  z.get<fruit, std::string, int, int>()["round fruit"].set<apple>();
  z.get<fruit, std::string, int, int>()["long fruit"].set<banana>();
  z.get<fruit, std::string, int, int>()["small fruit"].set<nectarine>();
  
}
BOOST_AUTO_TEST_CASE(construct_from_zone)
{
  factory_map z;
  setup_zone(z);
  std::vector<std::pair<std::string, factory<fruit, int, int> > >
          f1(z.get<fruit, std::string, int, int>().begin(), 
             z.get<fruit, std::string, int, int>().end());

  std::vector<std::pair<std::string, factory<fruit, int, int> > >
          f2(z.get<fruit, std::string, int, int>().begin(), 
             z.get<fruit, std::string, int, int>().end());
  std::map<std::string, factory<fruit, int, int> > m1(z);
  //std::vector<std::pair<std::string, factory<fruit, int, int> > > f3(m1);
  //std::vector<std::pair<std::string, factory<fruit, int, int> > > f4 = m1;
  BOOST_CHECK_EQUAL(f1.size(), f2.size());
  // BOOST_CHECK_EQUAL(f1.size(), f3.size());
  // BOOST_CHECK_EQUAL(f2.size(), f4.size());
  BOOST_CHECK_EQUAL(f1.size(), size_t(3));
}
BOOST_AUTO_TEST_CASE(factory_construction)
{
  factory_map z;
  setup_zone(z);
  std::vector<std::pair<std::string, factory<fruit, int, int> > > 
          f1(z.get<fruit, std::string, int, int>().begin(), 
             z.get<fruit, std::string, int, int>().end());
  std::vector<std::pair<std::string, factory<fruit, int, int> > >::iterator 
          it = f1.begin();
  // Since our TypeInfo is string, they will be created
  // in alphabetical order by TypeInfo ("round fruit" etc.), yielding
  // banana, apple, nectarine
  std::auto_ptr<fruit> first(it->second.create(0, 1));
  std::auto_ptr<fruit> second((++it)->second.create(0, 1));
  std::auto_ptr<fruit> third((++it)->second.create(0, 1));
  BOOST_CHECK_EQUAL((first->get_cost()), 7);
  BOOST_CHECK_EQUAL((second->get_cost()), 21);
  BOOST_CHECK_EQUAL((third->get_cost()), 18);
  BOOST_CHECK_EQUAL(typeid(*first.get()).name(), 
                    typeid(banana).name());
  BOOST_CHECK_EQUAL(typeid(*second.get()).name(), typeid(apple).name());
  BOOST_CHECK_EQUAL(typeid(*third.get()).name(), typeid(nectarine).name());
  BOOST_CHECK(typeid(*third.get()).name() != typeid(banana).name());
  BOOST_CHECK(typeid(*third.get()).name() != typeid(banana).name());
}

