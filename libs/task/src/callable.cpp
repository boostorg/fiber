
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/task/callable.hpp"

namespace boost {
namespace tasks {

callable::callable() :
	base_()
{}

void
callable::operator()()
{ base_->run(); }

bool
callable::empty() const
{ return ! base_; }

void
callable::clear()
{ base_.reset(); }

void
callable::reset( shared_ptr< thread > const& thrd)
{ base_->reset( thrd); }

void
callable::swap( callable & other)
{ base_.swap( other.base_); }

}}
