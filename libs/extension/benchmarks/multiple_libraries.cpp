/*
 * Boost.Extension / multiple libraries benchmark
 *         This benchmark loads a lot of libraries (comparing dl* and extensions)
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
#include <boost/timer.hpp>

#include <boost/filesystem/operations.hpp>
#include <boost/lexical_cast.hpp>

#include <iostream>
#include <string>
#include <memory>

#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif

#ifndef WINDOWS_LEAN_AND_MEAN
#define WINDOWS_LEAN_AND_MEAN
#endif

#include <Windows.h>
#   pragma comment(lib, "kernel32.lib")
#else
#include <dlfcn.h>
#endif


#include "../examples/word.hpp"


// copy the original library qty times to be loaded in the main routine
void copy_libraries(const std::string &lib_base, unsigned int qty)
{
  for(unsigned int i = 1; i <= qty; ++i) {
    std::string library_copy = lib_base
      + boost::lexical_cast<std::string>(i)
      + ".extension";

    if(boost::filesystem::exists(lib_base + ".extension") &&
       !boost::filesystem::exists(library_copy)) {

      boost::filesystem::copy_file(lib_base + ".extension", library_copy);
    }
  }
}


// remove the libraries after using them
void remove_libraries(const std::string &lib_base, unsigned int qty)
{
  for(unsigned int i = 1; i <= qty; ++i) {
    std::string library_copy = lib_base
      + boost::lexical_cast<std::string>(i)
      + ".extension";

    if(boost::filesystem::exists(library_copy)) {
      boost::filesystem::remove(library_copy);
    }
  }
}


int main(void)
{
  using namespace boost::extensions;

  unsigned int libs = 500;

  copy_libraries("libHelloWorldLib", libs);
  copy_libraries("libPlainOldHelloWorldLib", libs);


  // boost.extensions style
  boost::timer extensions_style;
  for(unsigned int lib_number = 1; lib_number <= libs; ++lib_number) {

    shared_library l(std::string("libHelloWorldLib"
                         + boost::lexical_cast<std::string>(lib_number)
                         + ".extension").c_str());

    l.open();
    {
      factory_map fm;
      void (*load_func)(factory_map &) =
        l.get<void, factory_map &>("extension_export_word");

      load_func(fm);

      std::map<int, factory<word> > & factory_list = fm.get<word, int>();
      for (std::map<int, factory<word> >::iterator current_word =
             factory_list.begin(); current_word != factory_list.end();
           ++current_word)
      {
        //  Using auto_ptr to avoid needing delete. Using smart_ptrs is
        // recommended.
        //  Note that this has a zero argument constructor - currently constructors
        //  with up to six arguments can be used.
        std::auto_ptr<word> word_ptr(current_word->second.create());
        std::string cheese = word_ptr->get_val();
      }
    }
    l.close();
  }
  std::cout << "Boost.extensions style: " << extensions_style.elapsed()
            << std::endl;


  // plain old style
  boost::timer old_style;
  for(unsigned int lib_number = 1; lib_number <= libs; ++lib_number) {

#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
    HMODULE library = LoadLibrary(std::string("libPlainOldHelloWorldLib"
                      + boost::lexical_cast<std::string>(lib_number)
                      + ".extension").c_str());
#else
    void *library = dlopen(std::string("libPlainOldHelloWorldLib"
                       + boost::lexical_cast<std::string>(lib_number)
                       + ".extension").c_str(), RTLD_LAZY);
#endif

    if(library == 0) {
      std::cerr << "Cannot open Hello World Library (libPlainOldHelloWorldLib"
                << lib_number << ".extension)" << std::endl;

      return 1;
    }
    typedef void (*export_words_function_type)(word **, word **);
    export_words_function_type export_words;

#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
    export_words = (export_words_function_type) GetProcAddress(library,
                                               "extension_export_words");
#else
    *(void **) (&export_words) = dlsym(library, "extension_export_words");
#endif

    if(export_words == 0) {
      std::cerr << "Cannot get exported symbol." << std::endl;
      return 1;
    }

    // retrieve the words
    word *first_word, *second_word;
    (*export_words)(&first_word, &second_word);

    // do something with the word
    std::string f(first_word->get_val());
    f += "\n";

    std::string s(second_word->get_val());
    s += "\n";

    delete first_word;
    delete second_word;

#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
    FreeLibrary(library);
#else
    dlclose(library);
#endif
  }
  std::cout << "Plain old style: " << old_style.elapsed() << std::endl;


  remove_libraries("libHelloWorldLib", libs);
  remove_libraries("libPlainOldHelloWorldLib", libs);

  return 0;
}
