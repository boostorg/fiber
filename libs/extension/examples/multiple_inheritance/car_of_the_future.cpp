/*
 * Boost.Extension / multiple inheritance example (car of the future)
 *
 * (C) Copyright Jeremy Pack 2007
 * Distributed under the Boost Software License, Version 1.0. (See             
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/ for latest version.
 */


/* The following lines are only necessary because when
are linking to this dll at compile time with another
dll on Windows. As such, standard __declspec stuff
is required.

This example is something of a special case - normally
these types of macros are not necessary for classes 
- see the FAQ.
  */
#include <boost/extension/extension.hpp>
#define BOOST_EXTENSION_CAR_OF_THE_FUTURE_DECL BOOST_EXTENSION_EXPORT_DECL

#include "car_of_the_future.hpp"
#include <boost/extension/type_map.hpp>
#include <boost/extension/factory.hpp>
#include <string>

std::string car_of_the_future::list_capabilities() {
  return boat::list_capabilities() + flying_car::list_capabilities() +
         computer::list_capabilities() + "\nCosts an arm and a leg";
}

using boost::extensions::factory;

BOOST_EXTENSION_TYPE_MAP_FUNCTION {
  types.get<std::map<std::string, factory<vehicle> > >()
    ["A car of the future exported as a vehicle"].set<car_of_the_future>();
  types.get<std::map<std::string, factory<car> > >()
    ["A car of the future exported as a car"].set<car_of_the_future>();
  types.get<std::map<std::string, factory<plane> > >()
    ["A car of the future exported as a plane"].set<car_of_the_future>();
  types.get<std::map<std::string, factory<flying_car> > >()
    ["A car of the future exported as a flying car"].set<car_of_the_future>();
  types.get<std::map<std::string, factory<boat> > >()
    ["A car of the future exported as a boat"].set<car_of_the_future>();
  types.get<std::map<std::string, factory<computer> > >()
    ["A car of the future exported as a computer"].set<car_of_the_future>();
  types.get<std::map<std::string, factory<car_of_the_future> > >()
    ["A car of the future exported as a car of the future"].set<car_of_the_future>();
}
