/*
 * Boost.Extension / counted factory test case
 *
 * (C) Copyright Jeremy Pack 2007
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/ for latest version.
 */

#include <boost/extension/counted_factory_map.hpp>

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

using namespace boost::extensions;

BOOST_AUTO_TEST_CASE(counted_factory_map_construction)
{
  counted_factory_map fm; 
}
