/*
 * Boost.Extension / registry class use example
 *
 * (C) Copyright Jeremy Pack 2007
 * Distributed under the Boost Software License, Version 1.0. (See             
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/ for latest version.
 */

#include "../word.hpp"
#include <boost/extension/registry.hpp>
#include <iostream>
using namespace boost::extensions;

int main(int argc, char * argv[])
{
  registry reg;
  reg.open("libRegistryLibrary.extension");
  {
    std::list<counted_factory<word, int> > & factory_list = 
      reg.get<word, int>(); 
    if (factory_list.size() != 1) {
      std::cout << "Expected to see one class available - found: " 
                << factory_list.size() << std::endl;
      return -1;
    }
    {
      std::auto_ptr<word> w(factory_list.begin()->create());
      const char * value;
      if (strcmp(value = w->get_val(), "First Time") != 0)
      {
        std::cout << "The string's value should have been \"First Time\" "
                  << "but was: " 
                  << value << std::endl;
        return -1;
      }
      if (strcmp(value = w->get_val(), "Second Time") != 0)
      {
        std::cout << "The string's value should have been \"Second Time\" "
                  << "but was: " 
                  << value << std::endl;
        return -1;
      }
    }
  }

  // This closes libregistry_library.extension, since none of 
  // its classes are instantiated any more.
  reg.clear();
  // Now we can repeat the above - with the same results.
  {
    reg.open("libRegistryLibrary.extension");
    std::list<counted_factory<word, int> > & factory_list = 
      reg.get<word, int>(); 
    if (factory_list.size() != 1) 
    {
      std::cout << "Expected to see one class available - found: " 
                << factory_list.size() << std::endl;
      return -1;
    }
    {
      std::auto_ptr<word> w(factory_list.begin()->create());
      const char * value;
      if (strcmp(value = w->get_val(), "First Time") != 0)
      {
        std::cout << "The string's value should have been \"First Time\" "
                  <<  "but was: " 
                  << value << std::endl;
        return -1;
      }
      if (strcmp(value = w->get_val(), "Second Time") != 0)
      {
        std::cout << "The string's value should have been \"Second Time\" "
                  << "but was: " 
                  << value << std::endl;
        return -1;
      }
    }
  }
}
