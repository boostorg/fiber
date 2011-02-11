
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_FIBER_SOURCE

#include <boost/fiber/detail/asym_fiber_base.hpp>

#include <boost/config.hpp>
#include <boost/assert.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

# if defined(BOOST_MSVC)
# pragma warning(push)
# pragma warning(disable:4355)
# endif

namespace boost {
namespace fibers {
namespace detail {

BOOST_FIBER_DECL void trampoline_asym( void * vp)
{
	BOOST_ASSERT( vp);
	detail::asym_fiber_base * self(
		static_cast< detail::asym_fiber_base * >( vp) );
	try
	{
		self->exec();
		self->set_finished();
	}
	catch (...)
	{ BOOST_ASSERT( false && "exeception from fiber-function"); }
}

asym_fiber_base::do_not_return_t asym_fiber_base::do_not_return;
asym_fiber_base::do_return_t asym_fiber_base::do_return;

asym_fiber_base::asym_fiber_base( std::size_t stacksize, do_not_return_t) :
	use_count_( 0),
	finished_( false),
	caller_(),
	callee_( trampoline_asym, this, protected_stack( stacksize) )
{}

asym_fiber_base::asym_fiber_base( std::size_t stacksize, do_return_t) :
	use_count_( 0),
	finished_( false),
	caller_(),
	callee_( trampoline_asym, caller_, this, protected_stack( stacksize) )
{}

void
asym_fiber_base::run()
{ caller_.jump_to( callee_); }

void
asym_fiber_base::yield()
{ callee_.jump_to( caller_); }

void
asym_fiber_base::set_finished()
{ finished_ = true; }

bool
asym_fiber_base::get_finished() const
{ return finished_; }

}}}

# if defined(BOOST_MSVC)
# pragma warning(pop)
# endif

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
