
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/fiber/fiber_group.hpp"

#include <boost/foreach.hpp>

#include "boost/fiber/operations.hpp"

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

fiber_group::~fiber_group()
{
    BOOST_FOREACH( fiber * f, fibers_)
    { delete f; }
}

bool
fiber_group::is_this_fiber_in()
{
    fiber::id id( this_fiber::get_id() );

    mutex::scoped_lock lk( mtx_);
    BOOST_FOREACH( fiber * f, fibers_)
    { if ( f->get_id() == id) return true; }

    return false;
}

bool
fiber_group::is_fiber_in( fiber * f)
{
    if ( ! f) return false;

    fiber::id id( f->get_id() );

    mutex::scoped_lock lk( mtx_);
    BOOST_FOREACH( fiber * f, fibers_)
    { if ( f->get_id() == id) return true; }

    return false;
}

void
fiber_group::add_fiber( fiber * f)
{
    if ( ! f) return;

    BOOST_ASSERT( ! is_fiber_in( f) );

    mutex::scoped_lock lk( mtx_);
    fibers_.push_back( f);
}

void
fiber_group::remove_fiber( fiber * f)
{
    if ( ! f) return;

    mutex::scoped_lock lk( mtx_);
    std::vector< fiber * >::iterator i(
        std::find( fibers_.begin(), fibers_.end(), f) );
    if ( fibers_.end() != i) fibers_.erase( i);
}

void
fiber_group::join_all()
{
    BOOST_ASSERT( ! is_this_fiber_in() );

    mutex::scoped_lock lk( mtx_);
    BOOST_FOREACH( fiber * f, fibers_)
    { if ( f->joinable() ) f->join(); }
}

void
fiber_group::interrupt_all()
{
    mutex::scoped_lock lk( mtx_);
    BOOST_FOREACH( fiber * f, fibers_)
    { f->interrupt(); }
}

std::size_t
fiber_group::size() const
{
    mutex::scoped_lock lk( mtx_);
    return fibers_.size();
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
