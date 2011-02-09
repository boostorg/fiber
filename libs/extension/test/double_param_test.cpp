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
  Automobile(int speed1, float speed2) : speed_(speed1 * speed2) {}
  virtual float getSpeed() {return speed_;}
protected:
  float speed_;
};

class Van : virtual public Automobile {
public:
  virtual ~Van() {}
  Van(int speed1, float speed2) : Automobile(speed1, speed2) {}
  virtual float getSpeed() {return speed_ / 2;}
};

class PickupTruck : virtual public Automobile {
public:
  virtual ~PickupTruck() {}
  PickupTruck(int speed1, float speed2) : Automobile(speed1, speed2) {}
  virtual float getSpeed() {return speed_ / 3;}
};

class Racecar : virtual public Automobile {
public:
  virtual ~Racecar() {}
  Racecar(int speed1, float speed2) : Automobile(speed1, speed2) {}
  virtual float getSpeed() {return 3 * speed_;}
};

class RacingVan : public Racecar, public Van
{
public:
  virtual ~RacingVan() {}
  RacingVan(int speed1, float speed2) : Automobile(speed1, speed2),
  Racecar(speed1, speed2), Van(speed1, speed2) {}
  virtual float getSpeed() {return 2 * speed_;}
};




BOOST_AUTO_TEST_CASE(factory_argless) {
  factory<Automobile, int, float> f;
  f.set<PickupTruck>();
  BOOST_CHECK(f.is_valid());
  std::auto_ptr<Automobile> pickup(f.create(45, 2.0f));
  BOOST_CHECK_EQUAL(pickup->getSpeed(), 30.0f);
}

BOOST_AUTO_TEST_CASE(map_argless)
{
  std::map<std::string, factory<Automobile, int, float> > m;
  m["A van"].set<Van>();
  m["A basic automobile"].set<Automobile>();
  m["A pickup truck"].set<PickupTruck>();
  std::auto_ptr<Automobile> van(m["A van"].create(10, 3.0f));
  BOOST_CHECK_EQUAL(van->getSpeed(), 15.0f);
  BOOST_CHECK_EQUAL
    (m["An unloaded car!"].create(30, 1.5f),
     (Automobile*)0);
}
/*
namespace boost { namespace extensions { namespace impl {
template <>
Automobile * create_function<Automobile, Racecar>::create()
{
  return new Racecar(25, 2.0f);
}
}}} // namespace boost::extensions::impl
*/

BOOST_AUTO_TEST_CASE(factory_template)
{
  factory<Automobile, int, float> f;
  f.set<Racecar>();
  BOOST_CHECK(f.is_valid());
  std::auto_ptr<Automobile> racecar(f.create(20, 3.0f));
  BOOST_CHECK_EQUAL(racecar->getSpeed(), 180.0f);
}

BOOST_AUTO_TEST_CASE(factory_map_single_param)
{
  factory_map m;
  m.get<Automobile, std::string, int, float>()["A pickup truck"]
    .set<PickupTruck>();
  m.get<Automobile, std::string, int, float>()["A racing van!"]
    .set<RacingVan>();
  std::auto_ptr<Automobile> pickup
    (m.get<Automobile, std::string, int, float>()["A pickup truck"]
     .create(30, 2.0f));
  BOOST_CHECK_EQUAL(pickup->getSpeed(), 20);
  std::auto_ptr<Automobile> racingVan
    (m.get<Automobile, std::string, int, float>()["A racing van!"]
     .create(20, 3.0f));
  BOOST_CHECK_EQUAL(racingVan->getSpeed(), 120);
  std::auto_ptr<Automobile> car_ptr
    (m.get<Automobile, std::string, int, float>()["A nonexistent car!"]
     .create(30, 1.2f));
  BOOST_CHECK_EQUAL(car_ptr.get(), (Automobile*)0);
}
