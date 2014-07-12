
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_DETAIL_TASK_OBJECT_H
#define BOOST_FIBERS_DETAIL_TASK_OBJECT_H

#include <boost/config.hpp>
#include <boost/throw_exception.hpp>

#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/future/detail/task_base.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace detail {

template< typename Fn, typename Allocator, typename R >
class task_object : public task_base< R >
{
public:
    typedef typename Allocator::template rebind<
        task_object< Fn, Allocator, R >
    >::other                                      allocator_t;

#ifdef BOOST_NO_RVALUE_REFERENCES
    task_object( Fn fn, allocator_t const& alloc) :
        task_base< R >(),
        fn_( fn),
        alloc_( alloc)
    {}
#endif

    task_object( BOOST_RV_REF( Fn) fn, allocator_t const& alloc) :
        task_base< R >(),
#ifdef BOOST_NO_CXX11_RVALUE_REFERENCES
        fn_( fn),
#else
        fn_( forward< Fn >( fn) ),
#endif
        alloc_( alloc)
    {}

    void run()
    {
        try
        {
            this->set_value( fn_() );
        }
        catch (...)
        {
            this->set_exception(
                current_exception() );
        }
    }

protected:
    void deallocate_future()
    { destroy_( alloc_, this); }

private:
    Fn                  fn_;
    allocator_t         alloc_;

    static void destroy_( allocator_t & alloc, task_object * p)
    {
        alloc.destroy( p);
        alloc.deallocate( p, 1);
    }
};

template< typename Fn, typename Allocator >
class task_object< Fn, Allocator, void > : public task_base< void >
{
public:
    typedef typename Allocator::template rebind<
        task_object< Fn, Allocator, void >
    >::other                                      allocator_t;

#ifdef BOOST_NO_RVALUE_REFERENCES
    task_object( Fn const& fn, allocator_t const& alloc) :
        task_base< void >(),
        fn_( fn),
        alloc_( alloc)
    {}
#endif

    task_object( BOOST_RV_REF( Fn) fn, allocator_t const& alloc) :
        task_base< void >(),
#ifdef BOOST_NO_CXX11_RVALUE_REFERENCES
        fn_( fn),
#else
        fn_( forward< Fn >( fn) ),
#endif
        alloc_( alloc)
    {}

    void run()
    {
        try
        {
            fn_();
            this->set_value();
        }
        catch (...)
        {
            this->set_exception(
                current_exception() );
        }
    }

protected:
    void deallocate_future()
    { destroy_( alloc_, this); }

private:
    Fn                  fn_;
    allocator_t         alloc_;

    static void destroy_( allocator_t & alloc, task_object * p)
    {
        alloc.destroy( p);
        alloc.deallocate( p, 1);
    }
};

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DETAIL_TASK_OBJECT_H
