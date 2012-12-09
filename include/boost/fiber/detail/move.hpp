// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// (C) Copyright 2007-8 Anthony Williams
// (C) Copyright 2011-2012 Vicente J. Botet Escriba

#ifndef BOOST_FIBERS_MOVE_HPP
#define BOOST_FIBERS_MOVE_HPP

#include <boost/fiber/detail/config.hpp>
#ifndef BOOST_NO_SFINAE
#include <boost/type_traits/decay.hpp>
#include <boost/type_traits/is_convertible.hpp>
#include <boost/type_traits/remove_cv.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/utility/enable_if.hpp>
#endif

#include <boost/config/abi_prefix.hpp>
#include <boost/move/move.hpp>
#include <boost/fiber/detail/delete.hpp>

#define BOOST_NOEXCEPT

namespace boost {
namespace detail {

template <typename T>
struct has_move_emulation_enabled_aux_dummy_specialization;
template<typename T>
struct fibers_move_t
{
    T& t;
    explicit fibers_move_t(T& t_):
        t(t_)
    {}

    T& operator*() const
    {
        return t;
    }

    T* operator->() const
    {
        return &t;
    }
private:
    void operator=(fibers_move_t&);
};

}

#ifndef BOOST_NO_SFINAE
template<typename T>
typename enable_if<boost::is_convertible<T&,boost::detail::fibers_move_t<T> >, boost::detail::fibers_move_t<T> >::type move(T& t)
{
    return boost::detail::fibers_move_t<T>(t);
}
#endif

template<typename T>
boost::detail::fibers_move_t<T> move(boost::detail::fibers_move_t<T> t)
{
    return t;
}

}

#if ! defined  BOOST_NO_RVALUE_REFERENCES

#define BOOST_FIBERS_RV_REF(TYPE) BOOST_RV_REF(TYPE)
#define BOOST_FIBERS_RV_REF_BEG BOOST_RV_REF_BEG
#define BOOST_FIBERS_RV_REF_END BOOST_RV_REF_END
#define BOOST_FIBERS_RV(V) V
#define BOOST_FIBERS_MAKE_RV_REF(RVALUE) RVALUE
#define BOOST_FIBERS_FWD_REF(TYPE) BOOST_FWD_REF(TYPE)
#define BOOST_FIBERS_DCL_MOVABLE(TYPE)
#define BOOST_FIBERS_DCL_MOVABLE_BEG(T) \
  namespace detail { \
    template <typename T> \
    struct has_move_emulation_enabled_aux_dummy_specialization<

#define BOOST_FIBERS_DCL_MOVABLE_END > \
      : integral_constant<bool, true> \
      {}; \
    }

#elif ! defined  BOOST_NO_RVALUE_REFERENCES && defined  BOOST_MSVC

#define BOOST_FIBERS_RV_REF(TYPE) BOOST_RV_REF(TYPE)
#define BOOST_FIBERS_RV_REF_BEG BOOST_RV_REF_BEG
#define BOOST_FIBERS_RV_REF_END BOOST_RV_REF_END
#define BOOST_FIBERS_RV(V) V
#define BOOST_FIBERS_MAKE_RV_REF(RVALUE) RVALUE
#define BOOST_FIBERS_FWD_REF(TYPE) BOOST_FWD_REF(TYPE)
#define BOOST_FIBERS_DCL_MOVABLE(TYPE)
#define BOOST_FIBERS_DCL_MOVABLE_BEG(T) \
  namespace detail { \
    template <typename T> \
    struct has_move_emulation_enabled_aux_dummy_specialization<

#define BOOST_FIBERS_DCL_MOVABLE_END > \
      : integral_constant<bool, true> \
      {}; \
    }

#else

#if defined BOOST_FIBERS_USES_MOVE
#define BOOST_FIBERS_RV_REF(TYPE) BOOST_RV_REF(TYPE)
#define BOOST_FIBERS_RV_REF_BEG BOOST_RV_REF_BEG
#define BOOST_FIBERS_RV_REF_END BOOST_RV_REF_END
#define BOOST_FIBERS_RV(V) V
#define BOOST_FIBERS_FWD_REF(TYPE) BOOST_FWD_REF(TYPE)
#define BOOST_FIBERS_DCL_MOVABLE(TYPE)
#define BOOST_FIBERS_DCL_MOVABLE_BEG(T) \
  namespace detail { \
    template <typename T> \
    struct has_move_emulation_enabled_aux_dummy_specialization<

#define BOOST_FIBERS_DCL_MOVABLE_END > \
      : integral_constant<bool, true> \
      {}; \
    }

#else

#define BOOST_FIBERS_RV_REF(TYPE) boost::detail::fibers_move_t< TYPE >
#define BOOST_FIBERS_RV_REF_BEG boost::detail::fibers_move_t<
#define BOOST_FIBERS_RV_REF_END >
#define BOOST_FIBERS_RV(V) (*V)
#define BOOST_FIBERS_FWD_REF(TYPE) BOOST_FWD_REF(TYPE)

#define BOOST_FIBERS_DCL_MOVABLE(TYPE) \
template <> \
struct has_move_emulation_enabled_aux< TYPE > \
  : BOOST_MOVE_BOOST_NS::integral_constant<bool, true> \
{};

#define BOOST_FIBERS_DCL_MOVABLE_BEG(T) \
template <typename T> \
struct has_move_emulation_enabled_aux<

#define BOOST_FIBERS_DCL_MOVABLE_END > \
  : BOOST_MOVE_BOOST_NS::integral_constant<bool, true> \
{};

#endif

namespace boost {
namespace detail {

template <typename T>
BOOST_FIBERS_RV_REF(typename ::boost::remove_cv<typename ::boost::remove_reference<T>::type>::type)
make_rv_ref(T v)  BOOST_NOEXCEPT
{
return (BOOST_FIBERS_RV_REF(typename ::boost::remove_cv<typename ::boost::remove_reference<T>::type>::type))(v);
}
//  template <typename T>
//  BOOST_FIBERS_RV_REF(typename ::boost::remove_cv<typename ::boost::remove_reference<T>::type>::type)
//  make_rv_ref(T &v)  BOOST_NOEXCEPT
//  {
//    return (BOOST_FIBERS_RV_REF(typename ::boost::remove_cv<typename ::boost::remove_reference<T>::type>::type))(v);
//  }
//  template <typename T>
//  const BOOST_FIBERS_RV_REF(typename ::boost::remove_cv<typename ::boost::remove_reference<T>::type>::type)
//  make_rv_ref(T const&v)  BOOST_NOEXCEPT
//  {
//    return (const BOOST_FIBERS_RV_REF(typename ::boost::remove_cv<typename ::boost::remove_reference<T>::type>::type))(v);
//  }
}}

#define BOOST_FIBERS_MAKE_RV_REF(RVALUE) RVALUE.move()
//#define BOOST_FIBERS_MAKE_RV_REF(RVALUE) boost::detail::make_rv_ref(RVALUE)
#endif


#if ! defined  BOOST_NO_RVALUE_REFERENCES

#define BOOST_FIBERS_MOVABLE(TYPE)

#else

#if defined BOOST_FIBERS_USES_MOVE

#define BOOST_FIBERS_MOVABLE(TYPE) \
    ::boost::rv<TYPE>& move()  BOOST_NOEXCEPT \
    { \
      return *static_cast< ::boost::rv<TYPE>* >(this); \
    } \
    const ::boost::rv<TYPE>& move() const BOOST_NOEXCEPT \
    { \
      return *static_cast<const ::boost::rv<TYPE>* >(this); \
    } \
    operator ::boost::rv<TYPE>&() \
    { \
      return *static_cast< ::boost::rv<TYPE>* >(this); \
    } \
    operator const ::boost::rv<TYPE>&() const \
    { \
      return *static_cast<const ::boost::rv<TYPE>* >(this); \
    }\

#else

#define BOOST_FIBERS_MOVABLE(TYPE) \
    operator ::boost::detail::fibers_move_t<TYPE>() BOOST_NOEXCEPT \
    { \
        return move(); \
    } \
    ::boost::detail::fibers_move_t<TYPE> move() BOOST_NOEXCEPT \
    { \
      ::boost::detail::fibers_move_t<TYPE> x(*this); \
        return x; \
    } \

#endif
#endif

#define BOOST_FIBERS_MOVABLE_ONLY(TYPE) \
  BOOST_FIBERS_NO_COPYABLE(TYPE) \
  BOOST_FIBERS_MOVABLE(TYPE) \

#define BOOST_FIBERS_COPYABLE_AND_MOVABLE(TYPE) \
  BOOST_FIBERS_MOVABLE(TYPE) \



#ifndef BOOST_NO_RVALUE_REFERENCES
namespace boost {
namespace fibers_detail {

template <class T>
typename decay<T>::type
decay_copy(T&& t)
{
  return boost::forward<T>(t);
}

}}
#endif

#include <boost/config/abi_suffix.hpp>

#endif
