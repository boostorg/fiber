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

#ifndef BOOST_EXTENSION_CAR_OF_THE_FUTURE_HPP
#define BOOST_EXTENSION_CAR_OF_THE_FUTURE_HPP
//  See the FAQ for info about why the following is necessary
//  here, but usually isn't.
#define BOOST_EXTENSION_FLYING_CAR_DECL BOOST_EXTENSION_IMPORT_DECL
#define BOOST_EXTENSION_BOAT_DECL BOOST_EXTENSION_IMPORT_DECL
#define BOOST_EXTENSION_COMPUTER_DECL BOOST_EXTENSION_IMPORT_DECL
#include "flying_car.hpp"
#include "boat.hpp"
#include "computer.hpp"
#include <iostream>
#include <typeinfo>
class BOOST_EXTENSION_CAR_OF_THE_FUTURE_DECL 
  car_of_the_future : public flying_car, public boat, public computer
{
public:
  car_of_the_future(void){std::cout << "\nCreated a Car of the Future";}
  ~car_of_the_future(void){std::cout << "\nDestroyed a Car of the Future";}
  virtual std::string list_capabilities(void);
};

#endif
