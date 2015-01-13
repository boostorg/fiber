
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_WORKER_FIBER_H
#define BOOST_FIBERS_WORKER_FIBER_H

#include <exception>
#include <functional>
#include <utility>

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/context/all.hpp>

#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/exceptions.hpp>
#include <boost/fiber/fiber_context.hpp>
#include <boost/fiber/fiber_manager.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

template< typename Fn >
class worker_fiber : public fiber_context {
private:
    Fn      fn_;

    void run() {
        try {
            BOOST_ASSERT( is_running() );
            fn_();
            BOOST_ASSERT( is_running() );
        } catch( fiber_interrupted const&) {
            set_exception( std::current_exception() );
        } catch( ... ) {
            std::terminate();
        }

        // mark fiber as terminated
        set_terminated();

        // notify waiting (joining) fibers
        release();

        // switch to another fiber
        fm_run();

        BOOST_ASSERT_MSG( false, "fiber already terminated");
    }

public:
    template< typename StackAlloc >
    explicit worker_fiber( context::preallocated palloc, StackAlloc salloc, Fn && fn) :
        fiber_context(
            context::execution_context( palloc, salloc, std::bind( & worker_fiber::run, this) ) ),
        fn_( std::forward< Fn >( fn) ) {
    }
};

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_WORKER_FIBER_H
