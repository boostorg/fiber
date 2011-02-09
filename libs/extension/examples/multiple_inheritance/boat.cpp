/*
 * Boost.Extension / multiple inheritance example (boat)
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
#define BOOST_EXTENSION_BOAT_DECL BOOST_EXTENSION_EXPORT_DECL
#include "boat.hpp"
#include <boost/extension/type_map.hpp>
#include <boost/extension/factory.hpp>


std::string boat::list_capabilities()
{
  return "\nIt floats on water.";
}

using boost::extensions::factory;

BOOST_EXTENSION_TYPE_MAP_FUNCTION {
  types.get<std::map<std::string, factory<vehicle> > >()
    ["A boat exported as a vehicle"].set<boat>();
  types.get<std::map<std::string, factory<boat> > >()
    ["A boat exported as a boat"].set<boat>();
}
