
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/task/context.hpp"

#include <boost/assert.hpp>

namespace boost {
namespace tasks {
namespace detail {

void
context_base::reset_( shared_ptr< thread > const& thrd)
{
	thrd_ = thrd;
	BOOST_ASSERT( thrd_);
	if ( requested_)
		if ( thrd_) thrd_->interrupt();
}

void
context_base::interrupt_()
{
	if ( ! requested_)
	{
		requested_ = true;
		if ( thrd_) thrd_->interrupt();
	}
}

context_base::context_base() :
	use_count_( 0),
	requested_( false),
	mtx_(),
	thrd_()
{}

void
context_base::reset( shared_ptr< thread > const& thrd)
{
	lock_guard< mutex > lk( mtx_);
	reset_( thrd);
}

void
context_base::interrupt()
{
	lock_guard< mutex > lk( mtx_);
	interrupt_();
}

bool
context_base::interruption_requested()
{
	lock_guard< mutex > lk( mtx_);
	return requested_;
}

}

context::context() :
	base_( new detail::context_base() )
{}

void
context::reset( shared_ptr< thread > const& thrd)
{ base_->reset( thrd); }

void
context::interrupt()
{ base_->interrupt(); }

bool
context::interruption_requested()
{ return base_->interruption_requested(); }

void
context::swap( context & other)
{ base_.swap( other.base_); }

}}
