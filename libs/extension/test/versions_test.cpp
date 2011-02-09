/*
 * Boost.Extension / implementations versions test
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
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK 1
#include <boost/test/unit_test.hpp>

#include <iostream>
#include <vector>
#include <string>

#include "../examples/word.hpp"
#include "../examples/versioning/salute.hpp"


BOOST_AUTO_TEST_CASE(versions_test)
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

  // the point here is to check if six classes were loaded 
  // (two for each one of the first three libraries)
  BOOST_CHECK_EQUAL( factory_list.size(), 6U );

  // these are the expected strings
  std::vector<std::string> words;
  words.push_back("hello");
  words.push_back("world!");
  words.push_back("| v2 hello");
  words.push_back("world! v2");
  words.push_back("| v2 hello");
  words.push_back("world! v2");

  std::vector<std::string>::const_iterator expected_word = words.begin();
  for (std::map<int, factory<word> >::iterator current_word = 
         factory_list.begin(); current_word != factory_list.end(); 
       ++current_word)
    {
      /// check that the pointer is OK and the word is the one that 
      /// we're expecting
      std::auto_ptr<word> word_ptr(current_word->second.create());
      BOOST_CHECK_EQUAL( !word_ptr.get(), 0 );
      BOOST_CHECK_EQUAL( word_ptr->get_val(), *expected_word);
      ++expected_word;
    }

  //  Get a reference to the list of constructors for salutes.
  std::map<int, factory<salute> > & salute_factory_list = fm.get<salute, 
    int>();  

  // the point here is to check if only two classes were loaded
  BOOST_CHECK_EQUAL( salute_factory_list.size(), 2U );

  // these are the expected strings
  std::vector<std::string> salutes;
  salutes.push_back("hello");
  salutes.push_back("bye!");

  std::vector<std::string>::const_iterator expected_salute = salutes.begin();

  for (std::map<int, factory<salute> >::iterator current_salute = 
         salute_factory_list.begin();
       current_salute != salute_factory_list.end(); ++current_salute)
    {
      /// check that the pointer is OK and the salute is the one that 
      /// we're expecting
      std::auto_ptr<salute> salute_ptr(current_salute->second.create());
      BOOST_CHECK_EQUAL( !salute_ptr.get(), 0 );
      BOOST_CHECK_EQUAL( salute_ptr->say(), *expected_salute);
      ++expected_salute;
    }

  // all ok
}
