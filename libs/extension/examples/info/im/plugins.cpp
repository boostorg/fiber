/*
 * Boost.Extension / IM plugins
 *
 * (C) Copyright Mariano G. Consoni 2007
 * Distributed under the Boost Software License, Version 1.0. (See             
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/ for latest version.
 */

#include <boost/extension/extension.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/extension/factory_map.hpp>

#include "protocol.hpp"
#include "network_parameters.hpp"

#include <iostream>

// MSN protocol implementation
class MSN : public protocol
{
public:
        virtual void login(const std::string &user, const std::string &pass) { 
          std::cout << "MSN: Logged In" << std::endl; 
        }
        virtual void send(const std::string &msg) { 
          std::cout << "MSN: message [" << msg << "] sent" << std::endl; 
        }
        virtual std::string receive(void) { 
          return std::string("MSN: hello! msg received"); 
        }
        virtual void change_status(const std::string &new_status) { 
          std::cout << "MSN: Status changed to [" << new_status << "]" 
                    << std::endl; 
        }

        virtual ~MSN(void) {}
};

// Jabber protocol implementation
class Jabber : public protocol
{
public:
        virtual void login(const std::string &user, const std::string &pass) { 
          std::cout << "Jabber: Logged In" << std::endl; 
        }
        virtual void send(const std::string &msg) { 
          std::cout << "Jabber: message [" << msg << "] sent" << std::endl; 
        }
        virtual std::string receive(void) { 
          return std::string("Jabber: hello! msg received"); 
        }
        virtual void change_status(const std::string &new_status) { 
          std::cout << "Jabber: Status changed to [" << new_status << "]" 
                    << std::endl; 
        }

        virtual ~Jabber(void) {}
};


extern "C" void BOOST_EXTENSION_EXPORT_DECL 
extension_export_plugins(boost::extensions::factory_map & fm)
{
  fm.get<protocol, boost::shared_ptr<network_parameters> >()
    [boost::shared_ptr<network_parameters>(new MSN_network_parameters)].set<MSN>();
  fm.get<protocol, boost::shared_ptr<network_parameters> >()
    [boost::shared_ptr<network_parameters>(new Jabber_network_parameters)]
    .set<Jabber>();
}
