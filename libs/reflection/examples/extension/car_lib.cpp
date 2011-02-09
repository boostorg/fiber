/*
 * Boost.Extension / car library implementations
 *
 * (C) Copyright Mariano G. Consoni 2008
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/ for latest version.
 */

#include "../car.hpp"
#include <boost/extension/extension.hpp>
#include <boost/reflection/reflection.hpp>
using namespace boost::reflections;

// Although both of these classes are derived from a common
// base, this is certainly not necessary. If we were using
// Boost.Extension factories, this would be required.
class suv {
public:
  suv(const char * name) {}
  const char* get_type(void) { return "It's an SUV."; }
  void start() {}
  bool start(int target_speed) {}
  short year;
};

class compact : public car
{
public:
        compact(const char * name) : car(name) {}
        virtual const char * get_type(void) { return "It's a compact."; }
        virtual ~compact(void) {}
};


extern "C" 
void BOOST_EXTENSION_EXPORT_DECL
extension_export_car(std::map<std::string, reflection> reflection_map) {
  reflection_map["suv"].reflect<suv>()
    .constructor<const char*>()
    .function(&suv::get_type, "get_type")
    .data(&suv::year, "year")
    .function<void>(&suv::start, "start")
    .function<bool, int>(&suv::start, "start");

  reflection_map["compact"].reflect<compact>()
    .constructor<const char*>()
    .function(&compact::get_type, "get_type");
}
