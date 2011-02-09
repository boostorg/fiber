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

#ifndef BOOST_REFLECTION_REFLECTION_HPP
#define BOOST_REFLECTION_REFLECTION_HPP

#include <map>
#include <vector>

#include <boost/extension/impl/typeinfo.hpp>
#include <boost/preprocessor/iteration/iterate.hpp>
#include <boost/reflection/constructor.hpp>
#include <boost/reflection/data.hpp>
#include <boost/reflection/data_info.hpp>
#include <boost/reflection/function.hpp>
#include <boost/reflection/parameter_map.hpp>

namespace boost {
namespace reflections {

// Provide a simplified version of the class for Doxygen, as
// well as for human consumption.
#ifdef BOOST_REFLECTION_DOXYGEN_INVOKED

template <class Info = std::string, class ParameterInfo = void,
          class TypeInfo = extensions::default_type_info>
class basic_reflection {
public:
typedef void ParameterInfo;
  /** \brief Internal class for reflecting constructors, functions etc.
    *
    * The reflector class is returned by the reflect method, and can
    * then be used to reflect constructors, functions or data.
    * It is normally used chained:
    * \code
    * reflection r;
    * r.reflect<MyClass>()
    *  .constructor()
    *  .constructor<int, float>();
    * \endcode
    */
  template <class T>
  class reflector
  {
  public:
    /** Initialize with a pointer to the reflection
      * this reflector will be reflecting into.
      * This is called by the reflection::reflect function.
      * \param current_reflection The reflection to set to type T.
      * \pre current_reflection has not already been reflected into.
      * \post None.
      */
    reflector(basic_reflection<Info, ParameterInfo, TypeInfo>*
              current_reflection)
      : reflection_(current_reflection) {
    }

    // Typedefs for the specific instantiations used by this class.
    typedef basic_function_info<Info, TypeInfo, ParameterInfo> function_info;
    typedef basic_constructor_info<TypeInfo, ParameterInfo> constructor_info;

    /** \brief Reflect a constructor with params (Params...)
      * Reflects the constructor of T that takes the arguments listed
      * in Params...
      * \tparam Params... A variable length list of parameters the
      *         constructor takes.
      * \returns *this.
      * \pre None.
      * \post None.
      */
    template <class Params...>
    reflector& constructor() {}

    /** \brief Reflect a data member of the class.
      * \param data_ptr The fully-qualified member address of the data member.
      * \param info A description or other Info of this data member.
      * \tparam Data The type of the data being reflected.
      * Reflect a data member of the class, which will be referenceable
      * by its type and the value of the Info parameter.
      * \code
      * reflection r;
      * r.reflect<MyClass>().constructor()
      *  .data(&MyClass:someData, "someDataDescription");
      * \endcode
      * \pre None.
      * \post None.
      */
    template <class Data>
    reflector& data(Data T::*data_ptr, Info info) {}

    /** \brief Reflect a member function of the class.
      * \param func The fully-qualified member address of the function.
      * \param info A description or other Info of this function.
      * \tparam Data The function's return type.
      * \tparam Params... The parameters of the function.
      * Reflect a member function of the class, which will be referenceable
      * by its type and the value of the Info parameter. The template
      * parameters only need to be included if the function is overloaded.
      * \code
      * reflection r;
      * r.reflect<MyClass>().constructor()
      *  .function(&MyClass:someFunction, "someFuncDescription");
      * \endcode
      * \pre None.
      * \post None.
      */
    template <class ReturnValue = void, class Params...>
    reflector& function(void (T::*func)(), Info info) {}
  };

  /** \brief Attempt to retrieve a constructor.
    * \tparam Params... The parameters of the requested function.
    * \returns A constructor reference (that must be checked for validity).
    *
    * Attempt to retrieve any constructor whose Params match the list in
    * Params...
    * For example:
    * \code
    * r.reflect<MyClass>()
    *  .constructor() ;
    * instance_constructor<> ic = r.get_constructor();
    * if (i.valid()) {
    *   instance i = ic();
    * }
    * \endcode
    * \pre None.
    * \post None.
    */
  template <class Params...>
  instance_constructor<Params...> get_constructor() const {}

  /** \brief Attempt to retrieve a data member.
    * \tparam Data The type of the requested data.
    * \returns A data reference (that must be checked for validity).
    *
    * Attempt to retrieve any data of type Data and the same Info.
    * For example:
    * \code
    * r.reflect<MyClass>()
    *  .constructor()
    *  .data(&MyClass:someInt, "My Int");
    * instance_constructor<> ic = r.get_constructor();
    * instance i = ic();
    * data<int> d = r.get_data<int>("My Int");
    * int& myIntRef = d(i);
    * \endcode
    * \pre None.
    * \post None.
    */
  template <class Data>
  data<Data> get_data(Info info) const {}

  /** \brief Set the type of this reflection.
    * \tparam T The type to set the reflection to.
    * Set the type of this reflection, and return a reflector
    * which can be used to reflect constructors, data and functions
    * of the class T.
    * \returns A reflector instance.
    * \pre reflect() has not been called.
    * \post None.
    */
  template <class T>
  reflector<T> reflect() {}
};

#else
using extensions::type_info_handler;
namespace impl {
template <class T>
void destruct(void * val) {
  delete static_cast<T*>(val);
}

#define BOOST_PP_ITERATION_LIMITS (0, \
    BOOST_PP_INC(BOOST_REFLECTION_MAX_FUNCTOR_PARAMS) - 1)
#define BOOST_PP_FILENAME_1 \
  <boost/reflection/impl/reflector_free_functions.hpp>
#include BOOST_PP_ITERATE()

// This is used to store, indexed by type information,
// any normal function pointers.
typedef void (*FunctionPtr)();

}  // namespace impl

// Since there are two specializations, with and without parameter
// information - but which are otherwise mostly the same - the
// implementation file is included twice.
#define BOOST_REFLECTION_WITH_PARAMETER_INFO
#include <boost/reflection/function_info.hpp>
#include <boost/reflection/constructor_info.hpp>
#include <boost/reflection/impl/reflection.hpp>
#undef BOOST_REFLECTION_WITH_PARAMETER_INFO

#include <boost/reflection/function_info.hpp>
#include <boost/reflection/constructor_info.hpp>
#include <boost/reflection/impl/reflection.hpp>
#endif  // BOOST_REFLECTION_DOXYGEN_INVOKED

typedef basic_reflection<> reflection;
}}
#endif  // BOOST_REFLECTION_REFLECTION_HPP
