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

// No header guard, as this file is intended to be included multiple times.

// By default, ParameterInfo is not used. Note that using adapters
// requires ParameterInfo.
#ifdef BOOST_REFLECTION_WITH_PARAMETER_INFO
template <class Info = std::string, class ParameterInfo = void,
          class TypeInfo = extensions::default_type_info>
class basic_reflection {
#else
template <class Info, class TypeInfo>
class basic_reflection<Info, void, TypeInfo> {
#endif
public:
#ifndef BOOST_REFLECTION_WITH_PARAMETER_INFO
typedef void ParameterInfo;
#endif
  // A reflector is used to add functions and constructors to
  // a reflected class.

  template <class T>
  class reflector
  {
  public:
    //  Initialize with a pointer to the reflection
    //  this reflector will be reflecting into
    reflector(basic_reflection<Info, ParameterInfo, TypeInfo>*
              current_reflection)
      : reflection_(current_reflection) {
    }
  
    // Typedefs for the specific instantiations used by this class.
    typedef basic_function_info<Info, TypeInfo, ParameterInfo> function_info;
    typedef basic_constructor_info<TypeInfo, ParameterInfo> constructor_info;
  
    reflector& constructor() {
      instance (*ctor_func)()(&impl::construct_instance<T>);
      reflection_->constructors_.insert(std::make_pair<TypeInfo, impl::FunctionPtr>(
          reflections::type_info_handler<TypeInfo, instance (*)()>
          ::get_class_type(), reinterpret_cast<impl::FunctionPtr>(ctor_func)));
      return *this;
    }

    template <class Data>
    reflector& data(Data T::*data_ptr, Info info) {
      data_info f(reflections::type_info_handler<TypeInfo, Data>
                    ::get_class_type(), info);
      Data& (*func)(void*, impl::MemberPtr) = &impl::get_data_from_ptr<T, Data>;
      std::pair<impl::MemberPtr, impl::FunctionPtr>
        p(reinterpret_cast<impl::MemberPtr>(data_ptr),
          reinterpret_cast<impl::FunctionPtr>(func));
      std::pair<data_info, std::pair<impl::MemberPtr, impl::FunctionPtr> >
        p2(f, p);
      reflection_->data_.insert(p2);
      return *this;
    }
  #define BOOST_PP_ITERATION_LIMITS (0, \
      BOOST_PP_INC(BOOST_REFLECTION_MAX_FUNCTOR_PARAMS) - 1)
  #define BOOST_PP_FILENAME_1 \
    <boost/reflection/impl/reflector_functions.hpp>
  #include BOOST_PP_ITERATE()
    template <class A>
    reflector& function(void (A::*func)(), Info info) {
#ifdef BOOST_REFLECTION_WITH_PARAMETER_INFO
      function_info f(reflections::type_info_handler<TypeInfo,
                        void (*)()>::get_class_type(), info, false);
#else
  function_info f(reflections::type_info_handler<TypeInfo,
                        void (*)()>::get_class_type(), info);
#endif
      void (*f2)(void *, impl::MemberFunctionPtr) = &impl::call_member<T, void>;
      std::pair<impl::MemberFunctionPtr, impl::FunctionPtr>
        in_pair(reinterpret_cast<impl::MemberFunctionPtr>(func),
          reinterpret_cast<impl::FunctionPtr>(f2));
      std::pair<function_info,
                std::pair<impl::MemberFunctionPtr,
                          impl::FunctionPtr> >
        out_pair(f, in_pair);
      reflection_->functions_.insert(out_pair);
      return *this;
    }
  private:
    basic_reflection<Info, ParameterInfo, TypeInfo>* reflection_;
  };
#define BOOST_PP_ITERATION_LIMITS (0, \
    BOOST_PP_INC(BOOST_REFLECTION_MAX_FUNCTOR_PARAMS) - 1)
#define BOOST_PP_FILENAME_1 <boost/reflection/impl/reflection_functions.hpp>
#include BOOST_PP_ITERATE()
  instance_constructor<> get_constructor() const {
    constructor_info t(reflections::type_info_handler<TypeInfo,
    instance (*)()>::get_class_type());
    typename std::map<constructor_info, impl::FunctionPtr>::const_iterator it =
      constructors_.find(t);
    if (it == constructors_.end()) {
      return instance_constructor<>();
    } else {
      return reinterpret_cast<instance (*)()>(it->second);
    }
  }

  template <class Data>
  data<Data> get_data(Info info) const {
    // Construct a data_info structure to look up the function in the map.
    data_info d(reflections::type_info_handler<TypeInfo, Data>
                            ::get_class_type(), info);
  
    // Look up the function.
    typename std::map<data_info,
      std::pair<impl::MemberPtr, impl::FunctionPtr> >::const_iterator it =
      data_.find(d);
  
    if (it == data_.end()) {
      // If it does not exist, return an empty function object.
      return data<Data>();
    } else {
      return data<Data>
        // reinterpret_cast is safe, because we looked it up by its type.
        (it->second.first,
         reinterpret_cast<Data& (*)(void*, impl::MemberPtr)>
           (it->second.second));
    }
  }

  template <class T>
  reflector<T> reflect() {
    return reflector<T>(this);
  }
private:
  typedef basic_function_info<Info, TypeInfo, ParameterInfo> function_info;
  typedef basic_constructor_info<TypeInfo, ParameterInfo> constructor_info;
  typedef basic_data_info<Info, TypeInfo> data_info;

  std::map<constructor_info, impl::FunctionPtr> constructors_;
  std::map<function_info,
    std::pair<impl::MemberFunctionPtr, impl::FunctionPtr> > functions_;
  std::map<data_info,
           std::pair<impl::MemberPtr, impl::FunctionPtr> > data_;
};
