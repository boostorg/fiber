
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_FIBER_SOURCE

#include <boost/fiber/sym_fiber.hpp>

#include <csignal>

#include <boost/config.hpp>
#include <boost/context/detail/stack_helper.hpp>

#include <boost/fiber/exceptions.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

std::size_t sym_fiber::max_stacksize = boost::contexts::detail::stack_helper::maximal_stacksize();
std::size_t sym_fiber::min_stacksize = boost::contexts::detail::stack_helper::minimal_stacksize();
std::size_t sym_fiber::default_stacksize = 256 * 1024;

sym_fiber
sym_fiber::from_current_context()
{ return sym_fiber( new detail::sym_fiber_dummy() ); }

sym_fiber::sym_fiber() :
	impl_()
{}

sym_fiber::sym_fiber( detail::sym_fiber_base::ptr const& impl) :
	impl_( impl)
{}

sym_fiber::sym_fiber( sym_fiber const& other) :
	impl_( other.impl_)
{}

sym_fiber &
sym_fiber::operator=( BOOST_COPY_ASSIGN_REF( sym_fiber) other)
{
	sym_fiber tmp( other);
	swap( tmp);
	return * this;
}

sym_fiber::sym_fiber( BOOST_RV_REF( sym_fiber) other) :
	impl_()
{ swap( other); }

sym_fiber &
sym_fiber::operator=( BOOST_RV_REF( sym_fiber) other)
{
	sym_fiber tmp( boost::move( other) );
	swap( tmp);
	return * this;
}

sym_fiber::operator unspecified_bool_type() const
{ return impl_; }

bool
sym_fiber::operator!() const
{ return ! impl_; }

bool
sym_fiber::operator==( sym_fiber const& other) const
{ return get_id() == other.get_id(); }

bool
sym_fiber::operator!=( sym_fiber const& other) const
{ return !( get_id() == other.get_id() ); }

void
sym_fiber::swap( sym_fiber & other)
{ impl_.swap( other.impl_); }

sym_fiber::id
sym_fiber::get_id() const
{ return sym_fiber::id( impl_); }

void
sym_fiber::switch_to( sym_fiber & other)
{
	if ( ! impl_ || ! other) throw fiber_moved();
	BOOST_ASSERT( ! impl_->get_finished() && "fiber already finished");
	BOOST_ASSERT( ! other.finished() && "fiber already finished");
	impl_->switch_to( * other.impl_);
}

bool
sym_fiber::finished() const
{
	if ( ! impl_) throw fiber_moved();
	return impl_->get_finished();
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
