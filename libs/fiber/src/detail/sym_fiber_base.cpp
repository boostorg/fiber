
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_FIBER_SOURCE

#include <boost/fiber/detail/sym_fiber_base.hpp>

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

BOOST_FIBER_DECL void trampoline_sym( void * vp)
{
	BOOST_ASSERT( vp);
	detail::sym_fiber_base * self(
		static_cast< detail::sym_fiber_base * >( vp) );
	try
	{
		self->exec();
		self->set_finished();
	}
	catch (...)
	{ BOOST_ASSERT( false && "exeception from fiber-function"); }
}

sym_fiber_base::sym_fiber_base() :
	use_count_( 0),
	finished_( false),
	ctx_()
{}

sym_fiber_base::sym_fiber_base( std::size_t stacksize) :
	use_count_( 0),
	finished_( false),
	ctx_( trampoline_sym, this, protected_stack( stacksize) )
{}

sym_fiber_base::sym_fiber_base( std::size_t stacksize, sym_fiber_base & nxt) :
	use_count_( 0),
	finished_( false),
	ctx_( trampoline_sym, nxt.ctx_, this, protected_stack( stacksize) )
{}

void
sym_fiber_base::switch_to( sym_fiber_base & other)
{ ctx_.jump_to( other.ctx_); }

void
sym_fiber_base::set_finished()
{ finished_ = true; }

bool
sym_fiber_base::get_finished() const
{ return finished_; }

}}}

# if defined(BOOST_MSVC)
# pragma warning(pop)
# endif

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
