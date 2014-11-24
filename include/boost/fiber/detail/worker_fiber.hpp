
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_DETAIL_WORKER_FIBER_H
#define BOOST_FIBERS_DETAIL_WORKER_FIBER_H

#include <exception> // std::terminate()

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/cstdint.hpp>
#include <boost/context/all.hpp>
#include <boost/context/execution_context.hpp>
#include <boost/move/move.hpp>

#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/fiber_base.hpp>
#include <boost/fiber/exceptions.hpp>
#include <boost/fiber/fiber_manager.hpp>
#include <boost/fiber/stack_context.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

# if defined(BOOST_MSVC)
# pragma warning(push)
# pragma warning(disable:4251)
# endif

namespace boost {
namespace fibers {
namespace detail {

class worker_fiber : public fiber_base
{
private:
    void run_()
    {
        try
        {
            BOOST_ASSERT( is_running() );
            fn_();
            BOOST_ASSERT( is_running() );
        }
        catch( fiber_interrupted const& )
        { except_ = current_exception(); }
        catch( ... )
        { std::terminate(); }

        // mark fiber as terminated
        set_terminated();
        // notify waiting (joining) fibers
        release();

        // switch to another fiber
        fibers::fm_run();

        BOOST_ASSERT_MSG( false, "fiber already terminated");
    }

public:
    template< typename StackAllocator, typename Fn >
    worker_fiber( StackAllocator const& salloc, Fn && fn_) :
        fiber_base(
            context::execution_context( salloc,
                                        [=,&fn_](){
                                            try
                                            {
                                                BOOST_ASSERT( is_running() );
                                                // store fiber-fn
                                                Fn fn( std::forward< Fn >( fn_) );
                                                fn_();
                                                BOOST_ASSERT( is_running() );
                                            }
                                            catch( fiber_interrupted const& )
                                            { except_ = current_exception(); }
                                            catch( ... )
                                            { std::terminate(); }

                                            // mark fiber as terminated
                                            set_terminated();
                                            // notify waiting (joining) fibers
                                            release();

                                            // switch to another fiber
                                            fibers::fm_run();

                                            BOOST_ASSERT_MSG( false, "fiber already terminated");
                                        })
    {}

    ~worker_fiber()
    { BOOST_ASSERT( is_terminated() ); }

    void deallocate()
    {
        BOOST_ASSERT( is_terminated() );

        StackAllocator salloc( salloc_);
        stack_context sctx( sctx_);
        // call destructor of worker_fiber
        this->~worker_fiber();
        // deallocate stack
        salloc.deallocate( sctx);
    }
};

}}}

# if defined(BOOST_MSVC)
# pragma warning(pop)
# endif

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DETAIL_WORKER_FIBER_H
