/*
 * Boost.Extension / dl* / extensions compare benchmark (with boost::function)
 *
 * (C) Copyright Mariano G. Consoni 2007
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/ for latest version.
 */

#define BOOST_EXTENSION_USE_BOOST_FUNCTION 1

// We include here a .cpp to be able to compile it with 
// the BOOST_EXTENSION_USE_BOOST_FUNCTION variable defined.
// In this way we can compare Boost.Extension with and
// without boost::function.
#include "plain_old_approach.cpp"
