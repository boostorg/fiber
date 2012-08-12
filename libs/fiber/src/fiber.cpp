
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_FIBERS_SOURCE

#include <boost/fiber/fiber.hpp>

#include <boost/assert.hpp>

#include <boost/fiber/detail/scheduler.hpp>
#include <boost/fiber/exceptions.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

fiber::operator unspecified_bool_type() const
{ return impl_ ? unspecified_bool : 0; }

bool
fiber::operator!() const
{ return ! impl_; }

bool
fiber::operator==( fiber const& other) const
{ return get_id() == other.get_id(); }

bool
fiber::operator!=( fiber const& other) const
{ return ! ( get_id() == other.get_id() ); }

void
fiber::swap( fiber & other)
{ impl_.swap( other.impl_); }

fiber::id
fiber::get_id() const
{ return impl_ ? impl_->get_id() : id(); }

bool
fiber::is_joinable() const
{ return impl_ && ! impl_->is_complete(); }

bool
fiber::is_complete() const
{
    BOOST_ASSERT( impl_);
	return impl_->is_complete();
}

void
fiber::cancel()
{
    BOOST_ASSERT( impl_);
    impl_->cancel();
}

bool
fiber::join()
{
    BOOST_ASSERT( impl_);
    if ( ! impl_->is_complete() )
        detail::scheduler::instance().join( impl_);
    BOOST_ASSERT( impl_->is_complete() );
    return ! impl_->is_canceled();
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
