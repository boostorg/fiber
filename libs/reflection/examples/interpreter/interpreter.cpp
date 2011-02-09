/*
 * Boost.Reflection / intepreter prototype example
 *
 * (C) Copyright Mariano G. Consoni 2007
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/ for latest version.
 */


#include <string>

#include <boost/extension/factory_map.hpp>
#include <boost/extension/shared_library.hpp>
#include <boost/extension/convenience.hpp>

#include <iostream>

#include "../car.hpp"
#include <boost/reflection/reflection.hpp>

#include <boost/regex.hpp>

#ifdef USE_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif


typedef std::list<boost::extensions::factory<car, std::string, 
                                             std::string> > factory_list;


/// this map stores the instanced variables
std::map<std::string, car *> context;

/// parse and execute a command
void parse_command(const std::string &line, factory_list &f,
                   boost::extension::reflection<car, 
                   std::string> &car_reflection)
{
  boost::regex re, call_re;
  boost::cmatch matches, call_matches;

  std::string creation_pattern("(\\w*)=(\\w*)\\((\\w*)\\)");
  std::string call_pattern("(\\w*).(\\w*)\\(\\)");

  try {
    re = creation_pattern;
  } catch (boost::regex_error& e) {
    std::cout << creation_pattern << " is not a valid regular expression: \""
              << e.what() << "\"" << std::endl;
    return;
  }

  try {
    call_re = call_pattern;
  } catch (boost::regex_error& e) {
    std::cout << call_pattern << " is not a valid regular expression: \""
              << e.what() << "\"" << std::endl;
    return;
  }


  if(boost::regex_match(line.c_str(), matches, re)) {

    if(matches.size() == 4) {
      std::string instance(matches[1].first, matches[1].second);
      std::string method(matches[2].first, matches[2].second);
      std::string parameter1(matches[3].first, matches[3].second);

      for (std::list<boost::extensions::factory<car, std::string, 
             std::string> >::iterator current_car = f.begin(); 
           current_car != f.end(); 
           ++current_car) {
         if(current_car->get_info() == method) {
          // FIXME: free
           car *created_car(current_car->create(parameter1));
          context[instance] = created_car;
          
          std::cout << "Instance [" << instance << "] created." << std::endl;
         }
      }
    }
    return;
  }


  if(boost::regex_match(line.c_str(), call_matches, call_re)) {

    if(call_matches.size() == 3) {
      std::string instance(call_matches[1].first, call_matches[1].second);
      std::string method(call_matches[2].first, call_matches[2].second);

      std::map<std::string, car *>::iterator m = context.find(instance);
      if(m != context.end()) {
        std::cout << "  --> " 
                  << car_reflection.call<std::string, bool>(m->second, method)
                  << std::endl;
      } else {
        std::cout << "Instance " << instance << " not found." << std::endl;
      }
    }
    return;
  }

  std::cout << "The command \"" << line << "\" is invalid." 
            << std::endl;

}


int main(void)
{
  using namespace boost::extensions;

  factory_map fm;
  load_single_library(fm, "libcar_lib.extension", 
                      "extension_export_car");
  std::list<factory<car, std::string, std::string> > & car_factory_list = 
          fm.get<car, std::string, std::string>();  
  if(car_factory_list.size() < 2) {
    std::cout << "Error - the classes were not found (" 
              << car_factory_list.size() << ").\n";
    std::exit(-1);
  }
 
  std::cout << std::endl 
            << " Boost.Reflection example - Prototype C++ interpreter."
            << std::endl << std::endl;

  boost::extension::reflection<car, std::string> car_reflection("A Car!");
  car_reflection.add<std::string, bool>(&car::start, "start");
  car_reflection.add<std::string, bool, float, 
    std::string>(&car::turn, "turn", "turn_angle");


  while(1) {
    std::string line;
    std::cout << "> ";
    std::cin >> line;

#ifdef USE_READLINE                
    char *line_chrptr = readline("> ");
    std::string line(line_chrptr);
    if(line.length() != 0) {
      add_history(line.c_str());
    }
#endif

    parse_command(line, car_factory_list, car_reflection);

#ifdef USE_READLINE                                  
    free(line_chrptr);
#endif
  }

  return 0;
}
