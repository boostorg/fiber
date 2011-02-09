/*
 * Boost.Extension / hello world example main
 *
 * (C) Copyright Jeremy Pack 2007
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
#include "word.hpp"

#include <memory>

int main()
{
  using namespace boost::extensions;
  //  Create the factory_map object - it will hold all of the available
  //  constructors. Multiple factory_maps can be constructed.
  factory_map fm;
  // load the shared library with 
  load_single_library(fm, "libHelloWorldLib.extension", 
                      "extension_export_word");
  //  Get a reference to the list of constructors for words.
  std::map<int, factory<word> > & factory_list = fm.get<word, int>();
  if (factory_list.size() < 2)
    std::cout << "Error - the classes were not found.";
  for (std::map<int, factory<word> >::iterator current_word = 
         factory_list.begin(); current_word != factory_list.end(); 
       ++current_word)
  {
    //  Using auto_ptr to avoid needing delete. Using smart_ptrs is 
    // recommended.
    //  Note that this has a zero argument constructor - currently constructors
    //  with up to six arguments can be used.
    std::auto_ptr<word> word_ptr(current_word->second.create());
    std::cout << word_ptr->get_val() << " ";
  }
  std::cout << "\n";
  return 0;
}
