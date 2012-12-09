// Copyright (C) 2012 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_DETAIL_DELETE_HPP
#define BOOST_FIBERS_DETAIL_DELETE_HPP

#include <boost/config.hpp>

/**
 * BOOST_FIBERS_DELETE_COPY_CTOR deletes the copy constructor when the compiler supports it or
 * makes it private.
 *
 * BOOST_FIBERS_DELETE_COPY_ASSIGN deletes the copy assignment when the compiler supports it or
 * makes it private.
 */
#ifndef BOOST_NO_DELETED_FUNCTIONS
#define BOOST_FIBERS_DELETE_COPY_CTOR(CLASS) \
      CLASS(CLASS const&) = delete; \

#define BOOST_FIBERS_DELETE_COPY_ASSIGN(CLASS) \
      CLASS& operator=(CLASS const&) = delete;

#else // BOOST_NO_DELETED_FUNCTIONS
#define BOOST_FIBERS_DELETE_COPY_CTOR(CLASS) \
    private: \
      CLASS(CLASS&); \
    public:

#define BOOST_FIBERS_DELETE_COPY_ASSIGN(CLASS) \
    private: \
      CLASS& operator=(CLASS&); \
    public:
#endif // BOOST_NO_DELETED_FUNCTIONS

/**
 * BOOST_FIBERS_NO_COPYABLE deletes the copy constructor and assignment when the compiler supports it or
 * makes them private.
 */
#define BOOST_FIBERS_NO_COPYABLE(CLASS) \
    BOOST_FIBERS_DELETE_COPY_CTOR(CLASS) \
    BOOST_FIBERS_DELETE_COPY_ASSIGN(CLASS)

#endif // BOOST_FIBERS_DETAIL_DELETE_HPP
