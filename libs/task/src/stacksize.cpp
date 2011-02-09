
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/task/stacksize.hpp"

#include <boost/task/exceptions.hpp>

namespace boost {
namespace tasks {

stacksize::stacksize( std::size_t value) :
	value_( value)
{ if ( value <= 0) throw invalid_stacksize(); }

stacksize::operator std::size_t () const
{ return value_; }

}}
