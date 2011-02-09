/*
 * Boost.Reflection / constructor information header
 *
 * (C) Copyright Mariano G. Consoni and Jeremy Pack 2008
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/ for latest version.
 */

// No header guards, as this header is intended to be included multiple times.

/** The basic_constructor_info class is used as a key in the map
  * of constructors available for the current reflection.
  * There are two types - those with ParameterInfo defined, and
  * those without.
  */
#ifdef BOOST_REFLECTION_WITH_PARAMETER_INFO
template<class TypeInfo, class Info = void>
struct basic_constructor_info {
/** A description for each parameter of the function.
  * If Info=void, this does not appear.
  * This member variable only occurs for reflections with
  * parameter info.
  */
  std::vector<Info> parameter_info_;
 private:
  function<instance ()> (*functor_func_)(
    boost::reflections::basic_parameter_map<Info>& map,
    const std::vector<Info>& names);
  instance (*func_)(
    boost::reflections::basic_parameter_map<Info>& map,
    const std::vector<Info>& names);
  std::map<TypeInfo, Info> (*check_func_)(
    const boost::reflections::basic_parameter_map<Info>& map,
    const std::vector<Info>& names);
  std::vector<Info> parameter_names_;
 public:
#else
template<class TypeInfo>
struct basic_constructor_info<TypeInfo> {
#endif
  // The type of the function pointer used to construct
  // the object this constructor_info is for.
  TypeInfo type_info_;

  // Constructors.
  explicit basic_constructor_info(TypeInfo t) : type_info_(t) {
  }

  basic_constructor_info(const basic_constructor_info & s)
    : type_info_(s.type_info_) {
  }

  basic_constructor_info& operator=(basic_constructor_info & s) {
    type_info_ = s.type_info_;
  }

  // Less than operator - for maps.
  friend inline bool operator<(const basic_constructor_info & t,
                               const basic_constructor_info & s) {
    return t.type_info_ < s.type_info_;
  }
};
