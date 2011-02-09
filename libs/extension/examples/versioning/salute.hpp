/*
 * Boost.Extension / salute interface
 *
 * (C) Copyright Mariano G. Consoni 2007
 * Distributed under the Boost Software License, Version 1.0. (See             
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/ for latest version.
 */

class salute
{
public:
  virtual ~salute(){}
  virtual const char * say(){return "";}
};
