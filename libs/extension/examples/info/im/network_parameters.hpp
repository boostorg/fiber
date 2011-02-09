/*
 * Boost.Extension / network parameters (info class)
 *
 * (C) Copyright Mariano G. Consoni 2007
 * Distributed under the Boost Software License, Version 1.0. (See             
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/ for latest version.
 */

#include <string>
#include <cstring>
#include <iostream>
#include <boost/shared_ptr.hpp>

// interface for the parameters of each plugin
class network_parameters
{
 public:
  virtual const char * hostname(void) const = 0;
  virtual const char * port(void) const = 0;
  virtual void set_http_mode(void) = 0;

  virtual ~network_parameters(void) {};
};


// MSN implementation
class MSN_network_parameters : public network_parameters
{
 public:
  virtual const char * hostname(void) const { return "msn.messenger.com"; }
  virtual const char * port(void) const { return "1863"; }

  virtual void set_http_mode(void) { 
    std::cout << "http mode set" << std::endl; 
  }

  virtual ~MSN_network_parameters() {}
};



// Jabber implementation
class Jabber_network_parameters : public network_parameters
{
 public:
  virtual const char * hostname(void) const { return "jabber.org"; }
  virtual const char * port(void) const { return "7063"; }

  virtual void set_http_mode(void) { 
    std::cout << "http mode not supported" << std::endl; 
  }

  virtual ~Jabber_network_parameters() {}
};

inline bool operator<(const boost::shared_ptr<network_parameters> & first,
                      const boost::shared_ptr<network_parameters> & second) {
  int comp = strcmp(first->hostname(), second->hostname());
  if (!comp) {
    return strcmp(first->port(), second->port()) < 0;
  }
  else return comp < 0;
}
