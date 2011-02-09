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


class Automobile {
public:
  virtual ~Automobile() {}
  Automobile() {}
  virtual int getSpeed() {return 45;}
};

class Van : virtual public Automobile {
public:
  virtual ~Van() {}
  Van() {}
  virtual int getSpeed() {return 35;}
};

class PickupTruck : virtual public Automobile {
public:
  virtual ~PickupTruck() {}
  PickupTruck() {}
  virtual int getSpeed() {return 50;}
};

class Racecar : virtual public Automobile {
public:
  virtual ~Racecar() {}
  Racecar(int speed) : speed_(speed) {}
  virtual int getSpeed() {return speed_;}
private:
  int speed_;
};

class RacingVan : public Racecar, public Van
{
public:
  virtual ~RacingVan() {}
  RacingVan() : Racecar(120) {}
  virtual int getSpeed() {return Racecar::getSpeed();}
};




BOOST_AUTO_TEST_CASE(factory_argless) {
  factory<Automobile> f;
  f.set<PickupTruck>();
  BOOST_CHECK(f.is_valid());
  std::auto_ptr<Automobile> pickup(f.create());
  BOOST_CHECK_EQUAL(pickup->getSpeed(), 50);
}

BOOST_AUTO_TEST_CASE(map_argless)
{
  std::map<std::string, factory<Automobile> > m;
  m["A van"].set<Van>();
  m["A basic automobile"].set<Automobile>();
  m["A pickup truck"].set<PickupTruck>();
  std::auto_ptr<Automobile> van(m["A van"].create());
  BOOST_CHECK_EQUAL(van->getSpeed(), 35);
  BOOST_CHECK_EQUAL
    (m["An unloaded car!"].create(),
     (Automobile*)0);
}
/*
namespace boost { namespace extensions {
template <>
Automobile * create_function<Automobile, Racecar>::create()
{
  return new Racecar(101);
}
}} // namespace boost::extensions

BOOST_AUTO_TEST_CASE(factory_template)
{
  factory<Automobile> f;
  f.set<Racecar>();
  BOOST_CHECK(f.is_valid());
  std::auto_ptr<Automobile> racecar(f.create());
  BOOST_CHECK_EQUAL(racecar->getSpeed(), 101);
}*/

BOOST_AUTO_TEST_CASE(factory_map_argless)
{
  factory_map m;
  m.get<Automobile, std::string>()["A pickup truck"].set<PickupTruck>();
  m.get<Automobile, std::string>()["A racing van!"].set<RacingVan>();
  std::auto_ptr<Automobile> pickup
    (m.get<Automobile, std::string>()["A pickup truck"].create());
  BOOST_CHECK_EQUAL(pickup->getSpeed(), 50);
  std::auto_ptr<Automobile> racingVan
    (m.get<Automobile, std::string>()["A racing van!"].create());
  BOOST_CHECK_EQUAL(racingVan->getSpeed(), 120);
  std::auto_ptr<Automobile> car_ptr
    (m.get<Automobile, std::string>()["A nonexistent car!"].create());
  BOOST_CHECK_EQUAL(car_ptr.get(), (Automobile*)0);
}
/*
BOOST_AUTO_TEST_CASE(registry_argless2)
{
  registry r;
  r.add<PickupTruck, Automobile, std::string>("A pickup truck");
  r.add<RacingVan, Automobile, std::string>("A racing van!");
  factory_map<Automobile, std::string> & m1(r.get<Automobile, std::string>());
  factory_map<Automobile, std::string> & m2(r.get());
  BOOST_CHECK_EQUAL(m1.size(), 2);
}

BOOST_AUTO_TEST_CASE(counted_registry_argless)
{
  count_registry r;
  r.add<PickupTruck, Automobile, std::string>("A pickup truck");
  r.add<RacingVan, Automobile, std::string>("A racing van!");
  {
    BOOST_CHECK_EQUAL(r.instances(), 0);
    std::auto_ptr<Automobile> pickup =
      r.create<Automobile, std::string>("A pickup truck");
    BOOST_CHECK_EQUAL(r.instances(), 1);
  }
  BOOST_CHECK_EQUAL(r.instances(), 0);
}*/
