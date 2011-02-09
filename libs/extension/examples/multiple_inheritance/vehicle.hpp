/*
 * Boost.Extension / multiple inheritance example (vehicle)
 *
 * (C) Copyright Jeremy Pack 2007
 * Distributed under the Boost Software License, Version 1.0. (See             
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/ for latest version.
 */

#ifndef BOOST_EXTENSION_VEHICLE_HPP
#define BOOST_EXTENSION_VEHICLE_HPP
#include <boost/extension/extension.hpp>
#include <iostream>
#include <typeinfo>
#include <string>

class BOOST_EXTENSION_VEHICLE_DECL vehicle
{
public:
  vehicle(void){std::cout << "\nCreated a Vehicle";}
  virtual ~vehicle(void){std::cout << "\nDestroyed a Vehicle";}
  virtual std::string list_capabilities(void);
};

#endif
