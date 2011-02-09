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

template <BOOST_PP_ENUM_PARAMS(N, class Param)>
class instance_constructor<BOOST_PP_ENUM_PARAMS(N, Param)> {
public:
  instance_constructor(instance (*func)(BOOST_PP_ENUM_PARAMS(N, Param)) = 0)
    : func_(func) {
  }
  instance call(BOOST_PP_ENUM_BINARY_PARAMS(N, Param, p)) {
    return (*func_)(BOOST_PP_ENUM_PARAMS(N, p));
  }
  instance operator()(BOOST_PP_ENUM_BINARY_PARAMS(N, Param, p)) {
    return (*func_)(BOOST_PP_ENUM_PARAMS(N, p));
  }
  bool valid() {return func_ != 0;}
private:
  instance (*func_)(BOOST_PP_ENUM_PARAMS(N, Param));
};

#undef N
