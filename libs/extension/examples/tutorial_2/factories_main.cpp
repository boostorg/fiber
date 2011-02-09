/*
 * Boost.Extension / hello world example main
 *
 * (C) Copyright Jeremy Pack 2008
 * Distributed under the Boost Software License, Version 1.0. (See             
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/ for latest version.
 */

#include <iostream>
#include <map>

#include <boost/extension/factory.hpp>
#include <boost/extension/shared_library.hpp>
#include <boost/extension/type_map.hpp>
#include <boost/function.hpp>
#include <boost/scoped_ptr.hpp>

#include "animal.hpp"

int main() {
  using namespace boost::extensions;

  // In the Jamfile, shared libraries are set to have the same
  // prefix and extension, even on different operating systems.
  // This is for convenience in writing cross-platform code, but
  // is not required. All shared libraries are set to start with
  // "lib" and end with "extension".
  std::string library_path = "libtutorial_2.extension";

  // Create shared_library object with the relative or absolute
  // path to the shared library.
  shared_library lib(library_path);

  // Attempt to open the shared library.
  if (!lib.open()) {
    std::cerr << "Library failed to open: " << library_path << std::endl;
    return 1;
  }

  // Use the shared_library::call to automatically call an
  // Extension-specific function in the shared library,
  // which takes a type_map.
  type_map types;
  if (!lib.call(types)) {
    std::cerr << "Function not found!" << std::endl;
    return 1;
  }

  // Retrieve a map of all animal factories taking an int and indexed
  // by a string from the type_map.
  std::map<std::string, factory<animal, int> >& factories(types.get());
  if (factories.empty()) {
    std::cerr << "Animals not found!" << std::endl;
    return 1;
  }

  // Create each animal.
  int age = 1;
  for (std::map<std::string, factory<animal, int> >::iterator it
         = factories.begin();
       it != factories.end(); ++it) {
    ++age;
    std::cout << "Creating an animal using factory: " << it->first << std::endl;
    boost::scoped_ptr<animal> current_animal(it->second.create(age));
    std::cout << "Created an animal: " << current_animal->get_name()
              << " Age: " << current_animal->get_age() << std::endl;
  }
}
