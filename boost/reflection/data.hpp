/*
 * Copyright Jeremy Pack 2008
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/ for latest version.
 */

#ifndef BOOST_REFLECTION_DATA_HPP
#define BOOST_REFLECTION_DATA_HPP

#include <boost/reflection/instance.hpp>

namespace boost {
namespace reflections {
namespace impl {
// This is used to store, indexed by type information,
// any member data pointers.
typedef void* instance::*MemberPtr;

template <class Object, class Data>
Data& get_data_from_ptr(void* inst, MemberPtr ptr) {
  Data Object::*data = reinterpret_cast<Data Object::*>(ptr);
  Object* obj = reinterpret_cast<Object*>(inst);
  return (obj->*data);
}
}  // namespace impl

template <class T>
class data {
public:
  data(impl::MemberPtr data_ptr = 0, 
       T& (*conversion_function)(void*, impl::MemberPtr) = 0)
    : data_ptr_(data_ptr),
      conversion_function_(conversion_function) {
  }
  T& operator()(instance & inst) const {
    return (*conversion_function_)(inst.val_, data_ptr_);
  }
  bool valid() const {
    return conversion_function_ != 0 && data_ptr_ != 0;
  }
private:
  impl::MemberPtr data_ptr_;
  T& (*conversion_function_)(void*, impl::MemberPtr);
};


}  // namespace reflections
}  // namespace boost
#endif  // BOOST_REFLECTION_DATA_HPP
