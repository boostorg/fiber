/*
 * Boost.Extension / hello world versions main
 *
 * (C) Copyright Mariano G. Consoni 2007
 * Distributed under the Boost Software License, Version 1.0. (See             
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/ for latest version.
 */

#include <boost/extension/extension.hpp>
#include <boost/extension/factory_map.hpp>
#include <boost/extension/shared_library.hpp>
#include <boost/extension/convenience.hpp>

#include <iostream>
#include <memory>


#include "../word.hpp"
#include "salute.hpp"

int main(void)
{
  using namespace boost::extensions;

  //  Create the factory_map object - it will hold all of the available
  //  constructors. Multiple factory_maps can be constructed.
  factory_map fm;

  // load hello world first version
  load_single_library(fm, "libHelloWorldLib.extension", 
                      "extension_export_word");

  // load hello world second version
  load_single_library(fm, "libHelloWorldLibv2.extension", 
                      "extension_export_word");

  // load hello world second version again
  load_single_library(fm, "libHelloWorldLibv2.extension", 
                      "extension_export_word");

  // load salute library (with hello included)
  load_single_library(fm, "libSaluteLib.extension", "extension_export_salute");

  //  Get a reference to the list of constructors for words.
  std::map<int, factory<word> > & factory_list = fm.get<word, int>();  

  if (factory_list.size() < 6) {
    std::cout << "Error - the classes were not found (" 
              << factory_list.size() << " classes)" << std::endl;
    return 1;
  }

  std::cout << "words: " << std::endl;
  for (std::map<int, factory<word> >::iterator current_word = 
         factory_list.begin();
       current_word != factory_list.end(); ++current_word)
    {
      //  Using auto_ptr to avoid needing delete. Using smart_ptrs is 
      //  recommended.
      //  Note that this has a zero argument constructor - currently 
      //  constructors
      //  with up to six arguments can be used.
      std::auto_ptr<word> word_ptr(current_word->second.create());
      std::cout << word_ptr->get_val() << " ";
    }
  std::cout << std::endl << std::endl;

  //  Get a reference to the list of constructors for salutes.
  std::map<int, factory<salute> > & salute_factory_list = 
    fm.get<salute, int>();  

  if (salute_factory_list.size() < 2) {
    std::cout << "Error - the classes were not found (" 
              << salute_factory_list.size() << " classes)" << std::endl;
    return 1;
  }

  std::cout << "salutes: " << std::endl;
  for (std::map<int, factory<salute> >::iterator current_salute = 
         salute_factory_list.begin();
       current_salute != salute_factory_list.end(); ++current_salute)
    {
      //  Using auto_ptr to avoid needing delete. Using smart_ptrs is
      //  recommended.
      //  Note that this has a zero argument constructor - currently 
      //  constructors
      //  with up to six arguments can be used.
      std::auto_ptr<salute> salute_ptr(current_salute->second.create());
      std::cout << salute_ptr->say() << " ";
    }
  std::cout << std::endl;

  return 0;
}
