/*
 * Copyright Jeremy Pack 2008
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/ for latest version.
 */


// No header guard - this file is intended to be included multiple times.

# define N BOOST_PP_ITERATION()
// Free functions. These should only be used internally by the
// reflector class to generate function pointers.
//
// This is a generic factory function to construct an instance of
// a given class using a constructor with the given signature.
template <class T BOOST_PP_COMMA_IF(N) BOOST_PP_ENUM_PARAMS(N, class Param)>
static instance construct_instance(BOOST_PP_ENUM_BINARY_PARAMS(N, Param, p)) {
  // An instance is similar to boost::any. Initialize it with
  // a void ptr.
  return instance(static_cast<void*>(
    construct<T BOOST_PP_COMMA_IF(N) BOOST_PP_ENUM_PARAMS(N, Param)>
     (BOOST_PP_ENUM_PARAMS(N, p))),
                  &destruct<T>);
}

// This auxiliary function is used to call a member function on
// a given instance, assuming the instance is of type T.
template <class T, class ReturnValue BOOST_PP_COMMA_IF(N)
          BOOST_PP_ENUM_PARAMS(N, class Param)>
static ReturnValue call_member(void * val,
                               impl::MemberFunctionPtr member_function
                               BOOST_PP_COMMA_IF(N)
                               BOOST_PP_ENUM_BINARY_PARAMS(N, Param, p)) {
  // Convert to a T*.
  T * actual = static_cast<T*>(val);

  // Convert the MemberFunctionPtr to the requested type.
  ReturnValue (T::*func)(BOOST_PP_ENUM_PARAMS(N, Param)) =
    reinterpret_cast<ReturnValue (T::*)(BOOST_PP_ENUM_PARAMS(N, Param))>
      (member_function);

  // Call the function and return the result.
  return (actual->*func)(BOOST_PP_ENUM_PARAMS(N, p));
}

// The following are versions of the above that don't require
// knowing their parameters.
/*
template <class Derived, class Info, class TypeInfo
          BOOST_PP_COMMA_IF(N) BOOST_PP_ENUM_PARAMS(N, class Param)>
instance create_func(
    boost::reflections::basic_parameter_map<Info, TypeInfo>& map,
    const std::vector<Info>& names) {
#if N
  reflections::generic_parameter<TypeInfo>* gen;
#define BOOST_REFLECTION_GET_FROM_LIST(z, n, data) \
  gen = map.template get_first<BOOST_PP_CAT(Param, n)>(names[n]); \
  if (!gen) return 0; \
  BOOST_PP_CAT(Param, n) BOOST_PP_CAT(p, n) = \
     gen->template cast<BOOST_PP_CAT(Param, n)>();
  BOOST_PP_REPEAT(N, BOOST_REFLECTION_GET_FROM_LIST, )
#undef BOOST_REFLECTION_GET_FROM_LIST
#endif  // N
  return new Derived(BOOST_PP_ENUM_PARAMS(N, p));
}

template <class Interface, class Derived, class Info, class TypeInfo
          BOOST_PP_COMMA_IF(N) BOOST_PP_ENUM_PARAMS(N, class Param)>
boost::function<instance ()> get_functor_func(
    boost::reflections::basic_parameter_map<Info, TypeInfo>& map,
    const std::vector<Info>& names) {
#if N
  reflections::generic_parameter<TypeInfo>* gen;
#define BOOST_REFLECTION_GET_FROM_LIST(z, n, data) \
  gen = map.template get_first<BOOST_PP_CAT(Param, n)>(names[n]); \
  if (!gen) return boost::function<Interface* ()>(); \
  BOOST_PP_CAT(Param, n) BOOST_PP_CAT(p, n) = \
     gen->template cast<BOOST_PP_CAT(Param, n)>();
  BOOST_PP_REPEAT(N, BOOST_REFLECTION_GET_FROM_LIST, )
#undef BOOST_REFLECTION_GET_FROM_LIST
#endif  // N
  Interface* (*f)(BOOST_PP_ENUM_PARAMS(N, Param)) =
    impl::create_derived<Interface, Derived BOOST_PP_COMMA_IF(N)
                         BOOST_PP_ENUM_PARAMS(N, Param)>;
  return bind(f
              BOOST_PP_COMMA_IF(N)
              BOOST_PP_ENUM_PARAMS(N, p));
}

template <class Info, class TypeInfo BOOST_PP_COMMA_IF(N)
          BOOST_PP_ENUM_PARAMS(N, class Param)>
inline std::map<TypeInfo, Info> check_func(
    const boost::reflections::basic_parameter_map<Info, TypeInfo>& map,
    const std::vector<Info>& names) {
  std::map<TypeInfo, Info> needed_parameters;
#define BOOST_REFLECTION_CHECK_IN_LIST(z, n, data) \
if (!map.template has<BOOST_PP_CAT(Param, n)>(names[n])) \
  needed_parameters.insert(std::make_pair(\
    type_info_handler<TypeInfo, \
                      BOOST_PP_CAT(Param, n)>::template get_class_type(), \
    names[n]));
  BOOST_PP_REPEAT(N, BOOST_REFLECTION_CHECK_IN_LIST, )
#undef BOOST_REFLECTION_CHECK_IN_LIST
  return needed_parameters;
}*/
#undef N
