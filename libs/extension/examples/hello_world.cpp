/*
 * Boost.Extension / hello world implementations
 *
 * (C) Copyright Jeremy Pack 2007-2008
 * Distributed under the Boost Software License, Version 1.0. (See             
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/ for latest version.
 */

#include "word.hpp"
#include <boost/extension/extension.hpp>
#include <boost/extension/factory_map.hpp>

class world : public word
{
public:
  virtual const char * get_val(){return "world!";}
};
class hello : public word
{
public:
  virtual const char * get_val(){return "hello";}
};
extern "C" 
void BOOST_EXTENSION_EXPORT_DECL 
extension_export_word(boost::extensions::factory_map & fm)
{
  fm.get<word, int>()[1].set<hello>();
  fm.get<word, int>()[2].set<world>();
}
