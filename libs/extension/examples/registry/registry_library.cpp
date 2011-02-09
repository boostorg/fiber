/*
 * Boost.Extension / registry class use example (library)
 *
 * (C) Copyright Jeremy Pack 2007
 * Distributed under the Boost Software License, Version 1.0. (See             
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/ for latest version.
 */

#include <boost/extension/counted_factory_map.hpp>
#include <boost/extension/extension.hpp>
// Notice that we don't need to include the registry file - just the class
// it's derived from. This keeps us from pulling the shared_library headers
// into this shared library.
#include "../word.hpp"
using namespace boost::extensions;
class counting_word : public word
{
public:
  virtual ~counting_word(){}
  virtual const char * get_val()
  {
    static int times_called = 0;
    ++times_called;
    switch (times_called)
    {
    case 1:
      return "First Time";
      break;
    case 2:
      return "Second Time";
      break;
    default:
      return "Many times";
      break;
    }
  }
};
// Using the default function name for loading registries:
extern "C" void BOOST_EXTENSION_EXPORT_DECL 
boost_extension_registry_function(counted_factory_map & fm)
{
  // 5 is just an identifier - not used in this example.
  // Arbitrary information (not just an int) can be stored
  // with the factory.
  fm.get<word, int>[5].set<counting_word>();
}
