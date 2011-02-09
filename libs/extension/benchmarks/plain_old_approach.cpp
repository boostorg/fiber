/*
 * Boost.Extension / dl* / extensions compare benchmark
 *
 * (C) Copyright Mariano G. Consoni 2007
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/ for latest version.
 */

#include <boost/extension/functor.hpp>
#include <boost/extension/factory_map.hpp>
#include <boost/extension/shared_library.hpp>
#include <boost/extension/convenience.hpp>
#include <boost/timer.hpp>

#include <iostream>
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


int main(void)
{
  using namespace boost::extensions;

  const unsigned int times = 1000;

  // boost.extensions style
  boost::timer extensions_style;
  for(unsigned int c = 0; c < times; ++c) {

    shared_library l((std::string("libHelloWorldLib") + ".extension").c_str());
    l.open();
    {
      factory_map fm;
      functor<void, factory_map &> load_func =
        l.get<void, factory_map &>("extension_export_word");
      load_func(fm);

      std::map<int, factory<word> > & factory_list = fm.get<word, int>();
      for (std::map<int, factory<word> >::iterator current_word =
             factory_list.begin(); current_word != factory_list.end();
           ++current_word) {
        std::auto_ptr<word> word_ptr(current_word->second.create());

        // do something with the word
        std::string s(word_ptr->get_val());
        s += "\n";
      }
    }
    l.close();
  }
  std::cout << "Boost.extensions style: " << extensions_style.elapsed()
            << std::endl;


  // plain old style
  boost::timer old_style;
  for(unsigned int c = 0; c < times; ++c) {

#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
    HMODULE library = LoadLibrary("libPlainOldHelloWorldLib.extension");
#else
    void *library = dlopen("libPlainOldHelloWorldLib.extension", RTLD_LAZY);
#endif

    if(library == 0) {
      std::cerr << "Cannot open Hello World Library." << std::endl;
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

  return 0;
}
