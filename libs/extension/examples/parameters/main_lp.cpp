/*
 * Boost.Extension / lots of parameters main
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
#include <iostream>
#include <boost/extension/convenience.hpp>

#include "lots_of_parameters_iface.hpp"

int main(void)
{
  using namespace boost::extensions;

  //  Create the factory_map object - it will hold all of the available
  //  constructors. Multiple factory_maps can be constructed.
  factory_map fm;

  // load the shared library with 
  load_single_library(fm, "libParametersLib.extension", "extension_export");

  std::map<int, factory<lots_of_parameters_interface, bool, unsigned int, 
    char, std::string, A, boost::shared_ptr<A> > > & factory_list = 
      fm.get<lots_of_parameters_interface, int, bool, unsigned int, char, 
      std::string, A, boost::shared_ptr<A>  >();
  if (factory_list.size() != 1) {
    std::cout << "Error - the class was not found.";
    return 1;
  }

  std::map<int, factory<lots_of_parameters_interface, bool, unsigned int, 
    char, std::string, A, boost::shared_ptr<A> > >::iterator par = 
    factory_list.begin();
  std::auto_ptr< lots_of_parameters_interface > 
    par_ptr(par->second.create(true, 4, 'c', "test", A(2), 
                        boost::shared_ptr<A>(new A(15))));

  return 0;
}
