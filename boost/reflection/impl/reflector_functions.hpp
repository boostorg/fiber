/*
 * Copyright Jeremy Pack 2008
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy atÄ
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/ for latest version.
 */


// No header guard - this file is intended to be included multiple times.

# define N BOOST_PP_ITERATION()
// Versions with included info about parameters
// An auxiliary macro to add a single parameter to a list
// of parameter information.
#define BOOST_REFLECTION_PUSH_PARAMETER_INFO_SINGLE(f, N) \
(f).parameter_info_.push_back(BOOST_PP_CAT(i, N));

// An auxiliary macro to add a series of parameters to a list
// of parameter information.
#define BOOST_REFLECTION_PUSH_PARAMETER_INFO(f, N) \
BOOST_PP_IF(N, BOOST_REFLECTION_PUSH_PARAMETER_INFO_SINGLE(f, BOOST_PP_DEC(N)),) \
BOOST_PP_IF(BOOST_PP_DEC(N), BOOST_REFLECTION_PUSH_PARAMETER_INFO(f, BOOST_PP_DEC(N)),)

// Reflect a constructor with the given signature.
public:
template <class ParamFirst BOOST_PP_COMMA_IF(N)
  BOOST_PP_ENUM_PARAMS(N, class Param)>
#ifdef BOOST_REFLECTION_WITH_PARAMETER_INFO
reflector& constructor(BOOST_PP_ENUM_PARAMS(N, ParameterInfo i)) {
#else
reflector& constructor() {
#endif
  instance (*ctor_func)(
    ParamFirst BOOST_PP_COMMA_IF(N) BOOST_PP_ENUM_PARAMS(N, Param))
      (&impl::construct_instance<T, ParamFirst
                                 BOOST_PP_COMMA_IF(N)
                                 BOOST_PP_ENUM_PARAMS(N, Param)>);
  constructor_info f(reflections::type_info_handler
      <TypeInfo, instance (*)(ParamFirst BOOST_PP_COMMA_IF(N)
                              BOOST_PP_ENUM_PARAMS(N, Param))>
        ::get_class_type());
#ifdef BOOST_REFLECTION_WITH_PARAMETER_INFO
  BOOST_REFLECTION_PUSH_PARAMETER_INFO(f, N);
#endif
  reflection_->constructors_.insert(
    std::make_pair<constructor_info, impl::FunctionPtr>(
      f, reinterpret_cast<impl::FunctionPtr>(ctor_func)));
  return *this;
}

// This version of the function is for reflecting functions that have
// return values - so that the name of the return value can be set.
template <class ReturnValue BOOST_PP_COMMA_IF(N)
          BOOST_PP_ENUM_PARAMS(N, class Param), class A>
#ifdef BOOST_REFLECTION_WITH_PARAMETER_INFO
reflector& function(ReturnValue (A::*func)(BOOST_PP_ENUM_PARAMS(N, Param)),
             Info info, ParameterInfo i_return BOOST_PP_COMMA_IF(N)
             BOOST_PP_ENUM_PARAMS(N, ParameterInfo i)) {
  // Create the function_info for this function.
  function_info f(reflections::type_info_handler<TypeInfo,
                  ReturnValue (*)(BOOST_PP_ENUM_PARAMS(N, Param))>
                    ::get_class_type(), info, true);
  // Add the ParameterInfo for each parameter to the function_info.
  BOOST_REFLECTION_PUSH_PARAMETER_INFO(f, N);
  // Add the ParameterInfo for the return type.
  f.parameter_info_.push_back(i_return);
#else
reflector& function(ReturnValue (A::*func)(BOOST_PP_ENUM_PARAMS(N, Param)),
                    Info info) {
    // Create the function_info for this function.
  function_info f(reflections::type_info_handler<TypeInfo,
                  ReturnValue (*)(BOOST_PP_ENUM_PARAMS(N, Param))>
                    ::get_class_type(), info);
#endif

  // Get a function pointer to a function that calls this member
  // function when given a void* that actually points to an instance
  // of this class.
  ReturnValue (*f2)(void *, impl::MemberFunctionPtr BOOST_PP_COMMA_IF(N)
      BOOST_PP_ENUM_PARAMS(N, Param)) =
      &impl::call_member<T, ReturnValue BOOST_PP_COMMA_IF(N)
                   BOOST_PP_ENUM_PARAMS(N, Param)>;

  // Create the pair objects to insert into the map.
  std::pair<impl::MemberFunctionPtr, impl::FunctionPtr>
    in_pair(reinterpret_cast<impl::MemberFunctionPtr>(func),
      reinterpret_cast<impl::FunctionPtr>(f2));
  std::pair<function_info, std::pair<impl::MemberFunctionPtr, impl::FunctionPtr> >
    out_pair(f, in_pair);
  reflection_->functions_.insert(out_pair);
  return *this;
}

// This version of the function is for reflecting functions that have
// no return value.
template <class ParamFirst BOOST_PP_COMMA_IF(N)
          BOOST_PP_ENUM_PARAMS(N, class Param), class A>
#ifdef BOOST_REFLECTION_WITH_PARAMETER_INFO
reflector& function(void (A::*func)(ParamFirst p_first BOOST_PP_COMMA_IF(N)
                             BOOST_PP_ENUM_PARAMS(N, Param)),
             Info info, ParameterInfo i_first BOOST_PP_COMMA_IF(N)
             BOOST_PP_ENUM_PARAMS(N, ParameterInfo i)) {
  function_info f(reflections::type_info_handler<TypeInfo,
                  void (*)(ParamFirst BOOST_PP_COMMA_IF(N)
                           BOOST_PP_ENUM_PARAMS(N, Param))>
                    ::get_class_type(), info, false);
  f.parameter_info_.push_back(i_first);
  BOOST_REFLECTION_PUSH_PARAMETER_INFO(f, N);
#else
reflector& function(void (A::*func)(ParamFirst p_first BOOST_PP_COMMA_IF(N)
                             BOOST_PP_ENUM_PARAMS(N, Param)),
                    Info info) {
   function_info f(reflections::type_info_handler<TypeInfo,
                  void (*)(ParamFirst BOOST_PP_COMMA_IF(N)
                           BOOST_PP_ENUM_PARAMS(N, Param))>
                    ::get_class_type(), info);
#endif
  void (*f2)(void *, impl::MemberFunctionPtr BOOST_PP_COMMA_IF(N)
      BOOST_PP_ENUM_PARAMS(N, Param)) =
      &impl::call_member<T, void BOOST_PP_COMMA_IF(N)
                   BOOST_PP_ENUM_PARAMS(N, Param)>;
  std::pair<impl::MemberFunctionPtr, impl::FunctionPtr>
    in_pair(reinterpret_cast<impl::MemberFunctionPtr>(func),
      reinterpret_cast<impl::FunctionPtr>(f2));
  std::pair<function_info, std::pair<impl::MemberFunctionPtr, impl::FunctionPtr> >
    out_pair(f, in_pair);
  reflection_->functions_.insert(out_pair);
  return *this;
}

#undef BOOST_REFLECTION_PUSH_PARAMETER_INFO_SINGLE
#undef BOOST_REFLECTION_PUSH_PARAMETER_INFO

#undef N
