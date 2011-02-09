/*
 * Boost.Extension / protocol interface
 *
 * (C) Copyright Mariano G. Consoni 2007
 * Distributed under the Boost Software License, Version 1.0. (See             
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/ for latest version.
 */

#include <string>
// protocol for instant message communication
class protocol
{
 public:

  virtual void login(const std::string &user, const std::string &pass) {}
  virtual void send(const std::string &msg) {}
  virtual std::string receive(void) { return std::string(""); }
  virtual void change_status(const std::string &new_status) {}

  virtual ~protocol(void) {}
};

