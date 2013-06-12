//
// use_ffuture.hpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_USE_FFUTURE_HPP
#define BOOST_ASIO_USE_FFUTURE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <memory>

#include <boost/asio/async_result.hpp>
#include <boost/asio/detail/config.hpp>
#include <boost/asio/detail/config.hpp>
#include <boost/asio/handler_type.hpp>
#include <boost/make_shared.hpp>
#include <boost/move/move.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <boost/thread/detail/memory.hpp>
#include <boost/throw_exception.hpp>

#include <boost/fiber/all.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {

/// Class used to specify that an asynchronous operation should return a future.
/**
 * The use_ffuture_t class is used to indicate that an asynchronous operation
 * should return a boost::fibers::future object. A use_ffuture_t object may be passed as a
 * handler to an asynchronous operation, typically using the special value @c
 * boost::asio::use_ffuture. For example:
 *
 * @code boost::fibers::future<std::size_t> my_future
 *   = my_socket.async_read_some(my_buffer, boost::asio::use_ffuture); @endcode
 *
 * The initiating function (async_read_some in the above example) returns a
 * future that will receive the result of the operation. If the operation
 * completes with an error_code indicating failure, it is converted into a
 * system_error and passed back to the caller via the future.
 */
template <typename Allocator = std::allocator<void> >
class use_ffuture_t
{
public:
  typedef Allocator allocator_type;

  /// Construct using default-constructed allocator.
  use_ffuture_t()
  {
  }

  /// Construct using specified allocator.
  explicit use_ffuture_t(const Allocator& allocator)
    : allocator_(allocator)
  {
  }

  /// Specify an alternate allocator.
  template <typename OtherAllocator>
  use_ffuture_t<OtherAllocator> operator[](const OtherAllocator& allocator) const
  {
    return use_ffuture_t<OtherAllocator>(allocator);
  }

  /// Obtain allocator.
  allocator_type get_allocator() const
  {
    return allocator_;
  }

private:
  Allocator allocator_;
};

/// A special value, similar to std::nothrow.
/**
 * See the documentation for boost::asio::use_ffuture_t for a usage example.
 */
BOOST_CONSTEXPR_OR_CONST use_ffuture_t<> use_ffuture;

} // namespace asio
} // namespace boost

namespace boost {
namespace asio {
namespace detail {

  // Completion handler to adapt a promise as a completion handler.
  template <typename T>
  class fpromise_handler
  {
  public:
    // Construct from use_ffuture special value.
    template <typename Allocator>
    fpromise_handler(use_ffuture_t<Allocator> uf)
      : promise_(boost::allocate_shared<boost::fibers::promise<T> >(
            uf.get_allocator(), boost::allocator_arg, uf.get_allocator()))
    {
    }

    void operator()(T t)
    {
      promise_->set_value(t);
      boost::fibers::detail::scheduler::instance().run();
    }

    void operator()(const boost::system::error_code& ec, T t)
    {
      if (ec)
        promise_->set_exception(
            boost::copy_exception(
              boost::system::system_error(ec)));
      else
        promise_->set_value(t);
      boost::fibers::detail::scheduler::instance().run();
    }

  //private:
    boost::shared_ptr<boost::fibers::promise<T> > promise_;
  };

  // Completion handler to adapt a void promise as a completion handler.
  template <>
  class fpromise_handler<void>
  {
  public:
    // Construct from use_ffuture special value. Used during rebinding.
    template <typename Allocator>
    fpromise_handler(use_ffuture_t<Allocator> uf)
      : promise_(boost::allocate_shared<boost::fibers::promise<void> >(
            uf.get_allocator(), boost::allocator_arg, uf.get_allocator()))
    {
    }

    void operator()()
    {
      promise_->set_value();
      boost::fibers::detail::scheduler::instance().run();
    }

    void operator()(const boost::system::error_code& ec)
    {
      if (ec)
        promise_->set_exception(
            boost::copy_exception(
              boost::system::system_error(ec)));
      else
        promise_->set_value();
      boost::fibers::detail::scheduler::instance().run();
    }

  //private:
    boost::shared_ptr<boost::fibers::promise<void> > promise_;
  };

  // Ensure any exceptions thrown from the handler are propagated back to the
  // caller via the future.
  template <typename Function, typename T>
  void asio_handler_invoke(Function f, fpromise_handler<T>* h)
  {
    boost::shared_ptr<boost::fibers::promise<T> > p(h->promise_);
    try
    {
      f();
    }
    catch (...)
    {
      p->set_exception(boost::current_exception());
    }
  }

} // namespace detail

#if !defined(GENERATING_DOCUMENTATION)

// Handler traits specialisation for fpromise_handler.
template <typename T>
class async_result<detail::fpromise_handler<T> >
{
public:
  // The initiating function will return a future.
  typedef boost::fibers::future<T> type;

  // Constructor creates a new promise for the async operation, and obtains the
  // corresponding future.
  explicit async_result(detail::fpromise_handler<T>& h)
  {
    value_ = h.promise_->get_future();
  }

  // Obtain the future to be returned from the initiating function.
  type get()
  {
      return boost::move(value_);
  }

private:
  type value_;
};

// Handler type specialisation for use_ffuture.
template <typename Allocator, typename ReturnType>
struct handler_type<use_ffuture_t<Allocator>, ReturnType()>
{
  typedef detail::fpromise_handler<void> type;
};

// Handler type specialisation for use_ffuture.
template <typename Allocator, typename ReturnType, typename Arg1>
struct handler_type<use_ffuture_t<Allocator>, ReturnType(Arg1)>
{
  typedef detail::fpromise_handler<Arg1> type;
};

// Handler type specialisation for use_ffuture.
template <typename Allocator, typename ReturnType>
struct handler_type<use_ffuture_t<Allocator>,
    ReturnType(boost::system::error_code)>
{
  typedef detail::fpromise_handler<void> type;
};

// Handler type specialisation for use_ffuture.
template <typename Allocator, typename ReturnType, typename Arg2>
struct handler_type<use_ffuture_t<Allocator>,
    ReturnType(boost::system::error_code, Arg2)>
{
  typedef detail::fpromise_handler<Arg2> type;
};

#endif // !defined(GENERATING_DOCUMENTATION)

} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_USE_FFUTURE_HPP
