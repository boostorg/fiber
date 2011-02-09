/*
 * Boost.Extension / multiple inheritance unit test
 *
 * (C) Copyright Mariano G. Consoni 2007
 * Distributed under the Boost Software License, Version 1.0. (See             
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/ for latest version.
 */

#include <boost/extension/factory_map.hpp>
#include <boost/extension/shared_library.hpp>
#include <boost/extension/convenience.hpp>

//  See the FAQ for info about why the following is necessary
//  here, but usually isn't.
#define BOOST_EXTENSION_VEHICLE_DECL BOOST_EXTENSION_IMPORT_DECL
#define BOOST_EXTENSION_COMPUTER_DECL BOOST_EXTENSION_IMPORT_DECL

#include "../examples/multiple_inheritance/vehicle.hpp"
#include "../examples/multiple_inheritance/computer.hpp"

#include <iostream>

#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK 1
#include <boost/test/unit_test.hpp>


using namespace boost::extensions;

BOOST_AUTO_TEST_CASE(multiple_inheritance_example)
{
  //  Create the factory_map object - it will hold all of the available
  //  constructors. Multiple zones can be constructed.
  factory_map twilight;

  // check if each library loads correct. It could be done with a helper 
  // function but I think that it is clear this way because we are in a 
  // unit test, and the output for the helper function version will be 
  // less descriptive

  // check if the libraries can be loaded
  shared_library libVehicle("libVehicle.extension");
  shared_library libCar("libCar.extension");
  shared_library libComputer("libComputer.extension");
  shared_library libBoat("libBoat.extension");
  shared_library libFlyingCar("libFlyingCar.extension");
  shared_library libCarOfTheFuture("libCarOfTheFuture.extension");
  shared_library libPlane("libPlane.extension");
  BOOST_CHECK_EQUAL( libVehicle.open(), true );
  BOOST_CHECK_EQUAL( libCar.open(), true );
  BOOST_CHECK_EQUAL( libComputer.open(), true );
  BOOST_CHECK_EQUAL( libBoat.open(), true );
  BOOST_CHECK_EQUAL( libFlyingCar.open(), true );
  BOOST_CHECK_EQUAL( libCarOfTheFuture.open(), true );
  BOOST_CHECK_EQUAL( libPlane.open(), true );

  // check if the factory can return the functor for each library
  void (*load_func_vehicle)(factory_map&) = 
    libVehicle.get<void, factory_map &>("extension_export");
  void (*load_func_car)(factory_map&) = 
    libCar.get<void, factory_map &>("extension_export");
  void (*load_func_computer)(factory_map&) = 
    libComputer.get<void, factory_map &>("extension_export");
  void (*load_func_boat)(factory_map&) = 
    libBoat.get<void, factory_map &>("extension_export");
  void (*load_func_flyingcar)(factory_map&) = 
    libFlyingCar.get<void, factory_map &>("extension_export");
  void (*load_func_carofthefuture)(factory_map&) =
    libCarOfTheFuture.get<void, factory_map &>("extension_export");
  void (*load_func_plane)(factory_map&) = 
    libPlane.get<void, factory_map &>("extension_export");
  BOOST_CHECK( load_func_vehicle != 0 );
  BOOST_CHECK( load_func_car != 0 );
  BOOST_CHECK( load_func_computer != 0 );
  BOOST_CHECK( load_func_boat != 0 );
  BOOST_CHECK( load_func_flyingcar != 0 );
  BOOST_CHECK( load_func_carofthefuture != 0 );
  BOOST_CHECK( load_func_plane != 0 );
  load_func_vehicle(twilight);
  load_func_car(twilight);
  load_func_computer(twilight);
  load_func_boat(twilight);
  load_func_flyingcar(twilight);
  load_func_carofthefuture(twilight);
  load_func_plane(twilight);


  // Computer test: we test if we obtain the computer implementation
  std::map<std::string, factory<computer> > & factory_list = 
    twilight.get<computer, std::string>();  
  BOOST_CHECK_EQUAL( factory_list.size(), 2U );

  std::map<std::string, factory<computer> >::iterator comp = 
    factory_list.begin();

  std::auto_ptr<computer> computer_ptr(comp->second.create());
  BOOST_CHECK_EQUAL( !computer_ptr.get(), 0 );

  BOOST_CHECK_EQUAL( comp->first, "\nA computer exported as a computer" );
  BOOST_CHECK_EQUAL( computer_ptr->list_capabilities(), "\nIt computes.");

  // Vehicles test: we test if we obtain the different vehicles implementation
  std::map<std::string, factory<vehicle> > & factory_list2 = 
    twilight.get<vehicle, std::string>();
  BOOST_CHECK_EQUAL( factory_list2.size(), 6U );

  std::map<std::string, factory<vehicle> >::iterator v = 
    factory_list2.begin();
  
  // boat as a vehicle
  std::auto_ptr<vehicle> v3_ptr(v->second.create());
  BOOST_CHECK_EQUAL( v->first,  "A boat exported as a vehicle" );
  BOOST_CHECK_EQUAL( v3_ptr->list_capabilities(), "\nIt floats on water." );
  
  ++v;
  
  // car as a vehicle
  std::auto_ptr<vehicle> v2_ptr(v->second.create());
  BOOST_CHECK_EQUAL( v->first,  "A car exported as a vehicle" );
  BOOST_CHECK_EQUAL( v2_ptr->list_capabilities(), "\nIt travels on roads." );

  ++v;
  
  // a car of the future as a vehicle
  std::auto_ptr<vehicle> v5_ptr(v->second.create());
  BOOST_CHECK_EQUAL( v->first,  "A car of the future exported as "
                     "a vehicle" );
  BOOST_CHECK_EQUAL( v5_ptr->list_capabilities(), "\nIt floats on water.\n"
                     "It travels on roads.\nIt flies in the air.\n"
                     "It takes off from your driveway\nIt computes.\n"
                     "Costs an arm and a leg" );
  
  ++v;
  
  // flying car as a vehicle
  std::auto_ptr<vehicle> v4_ptr(v->second.create());
  BOOST_CHECK_EQUAL( v->first,  "A flying car exported as a vehicle");
  BOOST_CHECK_EQUAL( v4_ptr->list_capabilities(), 
                     "\nIt travels on roads.\nIt flies in the air.\n"
                     "It takes off from your driveway" );

  ++v;

  // a plane as a vehicle
  std::auto_ptr<vehicle> v6_ptr(v->second.create());
  BOOST_CHECK_EQUAL( v->first,  "A plane exported as a vehicle" );
  BOOST_CHECK_EQUAL( v6_ptr->list_capabilities(), "\nIt flies in the air.");
  
  ++v;
  
  // vehicle as a vehicle
  std::auto_ptr<vehicle> v1_ptr(v->second.create());
  BOOST_CHECK_EQUAL( v->first,  "A vehicle exported as a vehicle" );
  BOOST_CHECK_EQUAL( v1_ptr->list_capabilities(), 
                     "\nIt is some sort of vehicle." );
  
  // all tests done
}
