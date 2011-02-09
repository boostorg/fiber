/*
 * Boost.Reflection / adapter (call functions using a parameter map)
 *
 * (C) Copyright Mariano G. Consoni 2008
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/ for latest version.
 */


#ifndef BOOST_REFLECTION_PARAMETER_MAP_HPP
#define BOOST_REFLECTION_PARAMETER_MAP_HPP
#include <boost/extension/impl/typeinfo.hpp>
#include <boost/reflection/parameter.hpp>
#include <exception>
#include <map>
#include <string>
#include <vector>

namespace boost { namespace reflections {
class parameter_unavailable_exception : public std::exception {
public:
  virtual const char * what() {
    return "Type not found in parameter_map";
  }
};

/** \brief A container for generic_parameters.
  *
  * The basic_parameter_map is designed to hold arbitrary data indexed
  * by the value of the Info parameter, and can return all items that
  * match a certain type and Info parameter, ie all integers called "foobar".
  */
template <class Info = std::string,
          class TypeInfo = extensions::default_type_info>
class basic_parameter_map
  : protected std::multimap<Info, generic_parameter<TypeInfo>*> {
public:
  /** The destructor deletes all of the parameters that it references.
    */
  ~basic_parameter_map() {
    for (typename map_type::iterator it = begin(); it != end(); ++it) {
      delete it->second;
    }
  }

  typedef std::multimap<Info, generic_parameter<TypeInfo>*> map_type;
  using map_type::equal_range;
  using map_type::begin;
  using map_type::end;
  using map_type::insert;

  /** \brief Return a vector of all parameters matching the given
    * type and Info.
    *
    * Given an Info value and a type, this will return all parameters
    * that either match the given type or are convertible to the given
    * type.
    * \return A vector of all parameters that match the given type and Info.
    * \param info The index or name of the parameters to return.
    * \tparam D The type of parameters to return.
    * \pre None.
    * \post None.
    */
  template <class D>
  std::vector<generic_parameter<TypeInfo>*> get(Info info) {
    std::vector<generic_parameter<TypeInfo>*> parameters;
    std::pair<typename map_type::iterator,
              typename map_type::iterator> its
      = equal_range(info);
    for (typename map_type::iterator current = its->first;
         current != its->second; ++current) {
      generic_parameter<TypeInfo>& p = *current->second;
      if (p.template can_cast<D>()) {
        parameters.push_back(current->second);
      }
    }
    return parameters;
  }

  /** \brief Return a vector of all parameters matching the given
    * type and Info.
    *
    * Given an Info value and a type, this will return all parameters
    * that either match the given type or are convertible to the given
    * type. This function is const.
    * \return A vector of all parameters that match the given type and Info.
    * \param info The index or name of the parameters to return.
    * \tparam D The type of parameters to return.
    * \pre None.
    * \post None.
    */
  template <class D>
  std::vector<const generic_parameter<TypeInfo>*> get(Info info) const {
    std::vector<generic_parameter<TypeInfo>*> parameters;
    std::pair<typename map_type::const_iterator,
              typename map_type::const_iterator> its
      = equal_range(info);
    for (typename map_type::const_iterator current = its->first;
         current != its->second; ++current) {
      const generic_parameter<TypeInfo>& p = *current->second;
      if (p.template can_cast<D>()) {
        parameters.push_back(current->second);
      }
    }
    return parameters;
  }
};

/** The most common version of basic_parameter_map.
  */
typedef basic_parameter_map<> parameter_map;
}}

#endif  // BOOST_REFLECTION_PARAMETER_MAP_HPP
