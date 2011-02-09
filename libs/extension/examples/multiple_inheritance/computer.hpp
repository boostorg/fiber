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

#ifndef BOOST_EXTENSION_COMPUTER_HPP
#define BOOST_EXTENSION_COMPUTER_HPP
#include <boost/extension/extension.hpp>
#include <iostream>
#include <string>
#include <typeinfo>
class BOOST_EXTENSION_COMPUTER_DECL computer
{
public:
  computer(void){std::cout << "\nCreated a Computer";}
  virtual ~computer(void){std::cout << "\nDestroyed a Computer";}
  virtual std::string list_capabilities(void);
};

#endif
