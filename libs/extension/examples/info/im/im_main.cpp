/*
 * Boost.Extension / instant messaging main
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
#include <boost/shared_ptr.hpp>

#include <iostream>

#include "protocol.hpp"
#include "network_parameters.hpp"


int main(void)
{
  using namespace boost::extensions;

  //  create the factory_map object - it will hold all of the available
  //  constructors. Multiple factory_maps can be constructed.
  factory_map fm;

  // load the shared library with 
  load_single_library(fm, "libIMPlugins.extension", 
                      "extension_export_plugins");

  //  get a reference to the list of constructors for protocols
   std::map<boost::shared_ptr<network_parameters>, factory<protocol> > & 
    factory_list = fm.get<protocol, boost::shared_ptr<network_parameters> >();

  if (factory_list.size() < 2) {
    std::cout << "Error - the classes were not found.";
    return 1;
  }

  std::map<boost::shared_ptr<network_parameters>, factory<protocol> >
    ::iterator current_plugin = factory_list.begin();

  // MSN plugin
  std::auto_ptr<protocol> MSN_ptr(current_plugin->second.create());
  boost::shared_ptr<network_parameters> msn_parameters = 
    current_plugin->first;
  current_plugin++;

  // Jabber plugin
  std::auto_ptr<protocol> Jabber_ptr(current_plugin->second.create());
  boost::shared_ptr<network_parameters> jabber_parameters = 
    current_plugin->first;

  // server
  std::cout << "MSN hostname: " << msn_parameters->hostname() << std::endl;
  std::cout << "Jabber hostname: " << jabber_parameters->hostname() 
            << std::endl;
  std::cout << std::endl;

  // http_mode: note that one of the implementations doesn't support it, 
  // having a base class
  // and different specific concrete network parameters allow us to handle this
  std::cout << "MSN: ";
  msn_parameters->set_http_mode();
  std::cout << "Jabber: ";
  jabber_parameters->set_http_mode();
  std::cout << std::endl;
  
  // login
  MSN_ptr->login("testuser", "testpass");
  Jabber_ptr->login("testuser", "testpass");
  std::cout << std::endl;
        
  // send message
  MSN_ptr->send("hi");
  Jabber_ptr->send("hi");
  std::cout << std::endl;
        
  // change status
  MSN_ptr->change_status("away");
  Jabber_ptr->change_status("away");
  std::cout << std::endl;

  // wait for message
  std::cout << MSN_ptr->receive() << std::endl;
  std::cout << Jabber_ptr->receive() << std::endl;
  std::cout << std::endl;

  return 0;
}
