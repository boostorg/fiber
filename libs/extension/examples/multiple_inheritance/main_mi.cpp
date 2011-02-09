/*
 * Boost.Extension / multiple inheritance example (main)
 *
 * (C) Copyright Jeremy Pack 2007
 * Distributed under the Boost Software License, Version 1.0. (See             
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/ for latest version.
 */

#include <boost/extension/type_map.hpp>
#include <boost/extension/shared_library.hpp>
//  See the FAQ for info about why the following is necessary
//  here, but usually isn't.
#define BOOST_EXTENSION_VEHICLE_DECL BOOST_EXTENSION_IMPORT_DECL
#define BOOST_EXTENSION_COMPUTER_DECL BOOST_EXTENSION_IMPORT_DECL
#include "vehicle.hpp"
#include "computer.hpp"
#include <iostream>
#include <boost/scoped_ptr.hpp>
#include <boost/extension/convenience.hpp>

int main() {
  using namespace boost::extensions;
  //  Create the type_map object - it will hold all of the available
  //  constructors.
  type_map types;
  //  Load the constructors and information into the factory_map.
  load_single_library(types, "libVehicle.extension");
  load_single_library(types, "libCar.extension");
  load_single_library(types, "libComputer.extension");
  load_single_library(types, "libBoat.extension");
  load_single_library(types, "libFlyingCar.extension");
  load_single_library(types, "libCarOfTheFuture.extension");
  load_single_library(types, "libPlane.extension");

  //  Get a reference to the list of constructors.
  //  Note that the factories can be copied just fine - meaning that the 
  //  map of factories can be copied from the type_map object into a
  // different data structure, and the type_map can be destroyed.
  // Here, we just use the efficient std::map::swap function.
  std::cout << "\n>>>>>>>>>>>>\nComputers:\n>>>>>>>>>>>>>>>>>>>";
  std::map<std::string, factory<computer> > computers;
  computers.swap(types.get());
  if (computers.empty()) {
    std::cout << "Error - no computers were found.";
    return 1;
  }
  for (std::map<std::string, factory<computer> >::iterator comp = 
       computers.begin(); comp != computers.end(); ++comp) {
    //  Using scoped_ptr to avoid needing delete. Using smart_ptrs is 
    //  recommended.
    //  Note that this has a zero argument constructor - currently constructors
    //  with up to six arguments can be used.
    boost::scoped_ptr<computer> computer_ptr(comp->second.create());
    std::cout << "\n--------\nLoaded the class described as: ";
    std::cout << comp->first;
    std::cout << "\n\nIt claims the following capabilities: ";
    std::cout << computer_ptr->list_capabilities() << std::endl;
  }
  std::cout << "\n\n";

  std::cout << "\n>>>>>>>>>>>>\nVehicles:\n>>>>>>>>>>>>>>>>>>>" << std::endl;
  std::map<std::string, factory<vehicle> > vehicles;
  vehicles.swap(types.get());
  if (vehicles.empty()) {
    std::cout << "Error - no vehicles were found.";
    return 1;
  }
  for (std::map<std::string, factory<vehicle> >::iterator v = 
       vehicles.begin(); v != vehicles.end(); ++v) {
    //  Using auto_ptr to avoid needing delete. Using smart_ptrs is 
    //  recommended.
    //  Note that this has a zero argument constructor - currently constructors
    //  with up to six arguments can be used.
    std::auto_ptr<vehicle> vehicle_ptr(v->second.create());
    std::cout << "\n--------\nLoaded the class described as: ";
    std::cout << v->first;
    std::cout << "\n\nIt claims the following capabilities: ";
    std::cout << vehicle_ptr->list_capabilities() << std::endl;
  }
  std::cout << "\n\n";  
  return 0;
}
