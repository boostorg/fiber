/*
 * Boost.Reflection / main header
 *
 * (C) Copyright Mariano G. Consoni and Jeremy Pack 2008
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/ for latest version.
 */

// No header guard, as this is intended to be included multiple times
// by reflection.hpp.

// The basic_function_info class is used as a key in the map
// of functions available for the current reflection.
// There are two types - those with ParameterInfo defined, and
// those without.
#ifdef BOOST_REFLECTION_WITH_PARAMETER_INFO
template<class Info, class TypeInfo, class ParameterInfo = void>
struct basic_function_info {
  // A description for each parameter of the function.
  // If ParameterInfo=void, this does not appear.
  std::vector<ParameterInfo> parameter_info_;
#else
// Same as the above, but without ParameterInfo.
template<class Info, class TypeInfo>
struct basic_function_info<Info, TypeInfo> {
#endif
  // The type of the function pointer in the map.
  TypeInfo type_info_;
  // A description of the function pointer.
  Info info_;

  bool has_return_;


  // Constructors
  basic_function_info(TypeInfo t, Info i, bool has_return = true)
    : type_info_(t), info_(i), has_return_(has_return) {
  }

  basic_function_info(const basic_function_info & s)
    : type_info_(s.type_info_), info_(s.info_) {
  }

  basic_function_info & operator=(basic_function_info & s) {
    type_info_ = s.type_info_;
    info_ = s.info_;
  }

  // Less-than operator - for maps.
  friend inline bool operator<(const basic_function_info & t,
                               const basic_function_info & s) {
    return t.type_info_ < s.type_info_ ||
    (t.type_info_ == s.type_info_ &&
     t.info_ < s.info_);
  }
};
