/*
 * Boost.Reflection / implementation header for Boost.PreProcessor
 *
 * (C) Copyright Jeremy Pack 2008
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/ for latest version.
 */
# define N BOOST_PP_ITERATION()
// No ifndef headers - this is meant to be included multiple times.

// Search for a constructor of the given type. instance_constructor
// has a method to determine if a suitable constructor was found.
template <class ParamFirst BOOST_PP_COMMA_IF(N)
          BOOST_PP_ENUM_PARAMS(N, class Param)>
instance_constructor<ParamFirst  BOOST_PP_COMMA_IF(N)
                     BOOST_PP_ENUM_PARAMS(N, Param)> get_constructor() const {
  // Create a constructor_info structure to use for looking up
  // a constructor in the constructor map. Initialize it with the
  // function type requested.
  constructor_info ctr_info(reflections::type_info_handler<TypeInfo,
                            instance (*)(ParamFirst BOOST_PP_COMMA_IF(N)
                                         BOOST_PP_ENUM_PARAMS(N, Param))>
                            ::get_class_type());

  // Determine whether or not such a constructor exists.
  typename std::map<constructor_info, impl::FunctionPtr>::const_iterator it =
    constructors_.find(ctr_info);

  if (it == constructors_.end()) {
    // If none exists, return an empty instance_constructor.
    return instance_constructor<ParamFirst  BOOST_PP_COMMA_IF(N)
                                BOOST_PP_ENUM_PARAMS(N, Param)>();
  } else {
    // reinterpret_cast is safe, because we looked it up by its type.
    return reinterpret_cast<instance (*)(ParamFirst BOOST_PP_COMMA_IF(N)
                                         BOOST_PP_ENUM_PARAMS(N, Param))>
      (it->second);
  }
}

// Search for a member function matching the given signature and Info.
template <class ReturnValue BOOST_PP_COMMA_IF(N)
          BOOST_PP_ENUM_PARAMS(N, class Param)>
function<ReturnValue BOOST_PP_COMMA_IF(N)
         BOOST_PP_ENUM_PARAMS(N, Param)> get_function(Info info) const {
  // Construct a function_info structure to look up the function in the map.
  // has_return is set to true here because it makes no difference when doing
  // a lookup in the map.
  function_info func_info(reflections::type_info_handler<TypeInfo,
                          ReturnValue (*)(BOOST_PP_ENUM_PARAMS(N, Param))>
                          ::get_class_type(), info);

  // Look up the function.
  typename std::map<function_info,
    std::pair<impl::MemberFunctionPtr, impl::FunctionPtr> >::const_iterator it =
    functions_.find(func_info);

  if (it == functions_.end()) {
    // If it does not exist, return an empty function object.
    return function<ReturnValue BOOST_PP_COMMA_IF(N)
                    BOOST_PP_ENUM_PARAMS(N, Param)>(); 
  } else {
    return function<ReturnValue BOOST_PP_COMMA_IF(N)
                    BOOST_PP_ENUM_PARAMS(N, Param)>
      // reinterpret_cast is safe, because we looked it up by its type.
      (reinterpret_cast<ReturnValue (*)(void *, impl::MemberFunctionPtr
                                        BOOST_PP_COMMA_IF(N)
                                        BOOST_PP_ENUM_PARAMS(N, Param))>
        (it->second.second), it->second.first);
  }
}

#undef N
