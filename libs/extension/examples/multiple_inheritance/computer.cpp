/*
 * Boost.Extension / multiple inheritance example (computer)
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
#define BOOST_EXTENSION_COMPUTER_DECL BOOST_EXTENSION_EXPORT_DECL


#include "computer.hpp"
#include <boost/extension/type_map.hpp>
#include <boost/extension/factory.hpp>

std::string computer::list_capabilities()
{
  return "\nIt computes.";
}

using boost::extensions::factory;

BOOST_EXTENSION_TYPE_MAP_FUNCTION {
  types.get<std::map<std::string, factory<computer> > >()
    ["\nA computer exported as a computer"].set<computer>();
}
