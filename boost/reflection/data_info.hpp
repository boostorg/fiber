/*
 * Boost.Reflection / data info
 *
 * (C) Copyright Jeremy Pack 2008
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/ for latest version.
 */

#ifndef BOOST_REFLECTION_DATA_INFO_HPP
#define BOOST_REFLECTION_DATA_INFO_HPP

#include <vector>
#include <boost/reflection/common.hpp>

namespace boost {
namespace reflections {

// The basic_data_info class is used as a key in the map
// of data available for the current reflection.
template<class Info, class TypeInfo>
struct basic_data_info {
  BOOST_CONCEPT_ASSERT((LessThanComparable<TypeInfo>));
  // The type of the function pointer in the map.
  TypeInfo type_info_;
  // A description of the function pointer.
  Info info_;

  // Constructors.
  basic_data_info(TypeInfo t, Info i)
    : type_info_(t), info_(i) {
  }

  basic_data_info(const basic_data_info & s)
    : type_info_(s.type_info_), info_(s.info_) {
  }

  basic_data_info & operator=(basic_data_info & s) {
    type_info_ = s.type_info_;
    info_ = s.info_;
  }

  // Less-than operator - for maps.
  friend inline bool operator<(const basic_data_info & t,
                               const basic_data_info & s) {
    return t.type_info_ < s.type_info_ ||
    (t.type_info_ == s.type_info_ &&
     t.info_ < s.info_);
  }
};

}  // namespace reflections
}  // namespace boost
#endif  // BOOST_REFLECTION_DATA_INFO_HPP
