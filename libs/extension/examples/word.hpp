/*
 * Boost.Extension / word interface
 *
 * (C) Copyright Jeremy Pack 2007
 * Distributed under the Boost Software License, Version 1.0. (See             
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/ for latest version.
 */

class word
{
public:
  virtual ~word(){}
  virtual const char * get_val(){return "";}
};
