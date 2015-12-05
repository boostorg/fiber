
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/fiber/exceptions.hpp"

namespace boost {
namespace fibers {

BOOST_FIBERS_DECL
std::error_category const& future_category() noexcept {
    static std::future_error_category cat;
    return cat;
}

}}
