
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_FIBER_SOURCE

#include <boost/fiber/asym_fiber.hpp>

#include <csignal>

#include <boost/config.hpp>
#include <boost/context/detail/stack_helper.hpp>

#include <boost/fiber/exceptions.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

std::size_t asym_fiber::max_stacksize = boost::contexts::detail::stack_helper::maximal_stacksize();
std::size_t asym_fiber::min_stacksize = boost::contexts::detail::stack_helper::minimal_stacksize();
std::size_t asym_fiber::default_stacksize = 256 * 1024;

asym_fiber::asym_fiber() :
	impl_()
{}

asym_fiber::asym_fiber( asym_fiber const& other) :
	impl_( other.impl_)
{}

asym_fiber &
asym_fiber::operator=( BOOST_COPY_ASSIGN_REF( asym_fiber) other)
{
	asym_fiber tmp( other);
	swap( tmp);
	return * this;
}

asym_fiber::asym_fiber( BOOST_RV_REF( asym_fiber) other) :
	impl_()
{ swap( other); }

asym_fiber &
asym_fiber::operator=( BOOST_RV_REF( asym_fiber) other)
{
	asym_fiber tmp( boost::move( other) );
	swap( tmp);
	return * this;
}

asym_fiber::operator unspecified_bool_type() const
{ return impl_; }

bool
asym_fiber::operator!() const
{ return ! impl_; }

bool
asym_fiber::operator==( asym_fiber const& other) const
{ return get_id() == other.get_id(); }

bool
asym_fiber::operator!=( asym_fiber const& other) const
{ return !( get_id() == other.get_id() ); }

void
asym_fiber::swap( asym_fiber & other)
{ impl_.swap( other.impl_); }

asym_fiber::id
asym_fiber::get_id() const
{ return asym_fiber::id( impl_); }

void
asym_fiber::run()
{
	if ( ! impl_) throw fiber_moved();
	BOOST_ASSERT( ! impl_->get_finished() && "fiber already finished");
	impl_->run();
}

void
asym_fiber::yield()
{
	if ( ! impl_) throw fiber_moved();
	BOOST_ASSERT( ! impl_->get_finished() && "fiber already finished");
	impl_->yield();
}

bool
asym_fiber::finished() const
{
	if ( ! impl_) throw fiber_moved();
	return impl_->get_finished();
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
