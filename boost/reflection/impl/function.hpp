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

template <class ReturnValue  BOOST_PP_COMMA_IF(N)
  BOOST_PP_ENUM_PARAMS(N, class Param)>
class function<ReturnValue BOOST_PP_COMMA_IF(N)
               BOOST_PP_ENUM_PARAMS(N, Param)> {
public:
  function(ReturnValue (*func)(void *, impl::MemberFunctionPtr
                        BOOST_PP_COMMA_IF(N)
                        BOOST_PP_ENUM_PARAMS(N, Param)) = 0,
           impl::MemberFunctionPtr member_function = 0)
  : func_(func),
    member_function_(member_function) {
  }
  ReturnValue call(instance & inst BOOST_PP_COMMA_IF(N)
                   BOOST_PP_ENUM_BINARY_PARAMS(N, Param, p)) const {
    return (*func_)(inst.val_, member_function_ BOOST_PP_COMMA_IF(N)
                    BOOST_PP_ENUM_PARAMS(N, p));
  }
  ReturnValue operator()(instance & inst BOOST_PP_COMMA_IF(N)
                         BOOST_PP_ENUM_BINARY_PARAMS(N, Param, p)) const {
    return (*func_)(inst.val_, member_function_ BOOST_PP_COMMA_IF(N)
                    BOOST_PP_ENUM_PARAMS(N, p));
  }
  bool valid() const {
    return member_function_ != 0 && func_ != 0;
  }
private:
  ReturnValue (*func_)(void *, impl::MemberFunctionPtr
                       BOOST_PP_COMMA_IF(N)
                       BOOST_PP_ENUM_PARAMS(N, Param));
  impl::MemberFunctionPtr member_function_;
};

#undef N
