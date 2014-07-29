
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_DETAIL_TRAMPOLINE_H
#define BOOST_FIBERS_DETAIL_TRAMPOLINE_H

#include <cstddef>
#include <exception>

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/coroutine/exceptions.hpp>
#include <boost/coroutine/symmetric_coroutine.hpp>
#include <boost/cstdint.hpp>
#include <boost/exception_ptr.hpp>
#include <boost/move/move.hpp>

#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/setup.hpp>
#include <boost/fiber/detail/worker_fiber.hpp>
#include <boost/fiber/exceptions.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace detail {

namespace coro = boost::coroutines;

template< typename Fn >
void trampoline( typename worker_fiber::coro_t::yield_type & yield)
{
    BOOST_ASSERT( yield);

    void * p( yield.get() );
    BOOST_ASSERT( p);
    setup< Fn > * from( static_cast< setup< Fn > * >( p) );

    // allocate instance of worker-fiber on its stack
    worker_fiber f( & yield);
    // return address of worker-fiber instance
    from->f = & f;

    try
    {
        // allocate fiber-function on worker-fiber's stack
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
        Fn fn( forward< Fn >( from->fn) );
#else
        Fn fn( move( from->fn) );
#endif

        // after setup return to caller
        f.set_running();
        f.suspend();

        // start executing fiber-function
        BOOST_ASSERT( f.is_running() );
        fn();
        BOOST_ASSERT( f.is_running() );
    }
    catch ( coro::detail::forced_unwind const&)
    {
        f.set_terminated();
        f.release();
        throw;
    }
    catch ( fiber_interrupted const&)
    { f.set_exception( current_exception() ); }
    catch (...)
    { std::terminate(); }

    f.set_terminated();
    f.release();
    f.suspend();

    BOOST_ASSERT_MSG( false, "fiber already terminated");
}

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DETAIL_TRAMPOLINE_H
