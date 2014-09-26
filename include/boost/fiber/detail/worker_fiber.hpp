
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

template< typename Fn, typename StackAllocator >
class worker_fiber : public fiber_base
{
private:
    Fn              fn_;
    StackAllocator  salloc_;
    stack_context   sctx_;

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
    static void entry_func( intptr_t);

#ifdef BOOST_NO_CXX11_RVALUE_REFERENCES
    worker_fiber( Fn fn, void * sp, std::size_t size,
                  StackAllocator const& salloc, stack_context const& sctx) :
        fiber_base( context::make_fcontext( sp, size, & worker_fiber::entry_func) ),
        fn_( boost::forward< Fn >( fn) ),
        salloc_( salloc),
        sctx_( sctx)
    {}
#endif

    worker_fiber( BOOST_RV_REF( Fn) fn, void * sp, std::size_t size,
                  StackAllocator const& salloc, stack_context const& sctx) :
        fiber_base( context::make_fcontext( sp, size, & worker_fiber::entry_func) ),
#ifdef BOOST_NO_CXX11_RVALUE_REFERENCES
        fn_( fn),
#else
        fn_( boost::forward< Fn >( fn) ),
#endif
        salloc_( salloc),
        sctx_( sctx)
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

template< typename Fn, typename StackAllocator >
void
worker_fiber< Fn, StackAllocator >::entry_func( intptr_t param)
{
    worker_fiber< Fn, StackAllocator > * f(
        reinterpret_cast< worker_fiber< Fn, StackAllocator > * >( param) );
    BOOST_ASSERT( 0 != f);
    f->run_();
}

}}}

# if defined(BOOST_MSVC)
# pragma warning(pop)
# endif

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DETAIL_WORKER_FIBER_H
