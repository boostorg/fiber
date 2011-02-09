/*
 * Copyright Jeremy Pack 2008
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/ for latest version.
 */

#ifndef BOOST_EXTENSION_GENERIC_CONSTRUCTOR_HPP
#define BOOST_EXTENSION_GENERIC_CONSTRUCTOR_HPP

namespace boost {
namespace reflections {
template <class T>
class generic_constructor {
public:
  virtual ~generic_constructor() {}
  virtual T * create(void ** params) const = 0;
};
}  // namespace reflections
}  // namespace boost
#endif
