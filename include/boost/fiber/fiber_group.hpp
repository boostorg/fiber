
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_FIBER_GROUP_H
#define BOOST_FIBERS_FIBER_GROUP_H

#include <cstddef>
#include <memory>
#include <vector>

#include <boost/config.hpp>
#include <boost/utility.hpp>

#include <boost/fiber/attributes.hpp>
#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/fiber.hpp>
#include <boost/fiber/mutex.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

# if defined(BOOST_MSVC)
# pragma warning(push)
# pragma warning(disable:4251 4275)
# endif

namespace boost {
namespace fibers {

class BOOST_FIBERS_DECL fiber_group : private noncopyable
{
public:
    fiber_group() {}

    ~fiber_group();

    bool is_this_fiber_in();

    bool is_fiber_in( fiber * f);

    template< typename Fn >
    fiber * create_fiber( Fn fn, attributes attrs = attributes() )
    {
        mutex::scoped_lock lk( mtx_);

        std::auto_ptr< fiber > f( new fiber( fn, attrs) );
        fibers_.push_back( f.get() );
        return f.release();
    }

    template< typename Fn, typename StackAllocator >
    fiber * create_fiber( Fn fn, attributes attrs,
                          StackAllocator const& stack_alloc)
    {
        mutex::scoped_lock lk( mtx_);

        std::auto_ptr< fiber > f( new fiber( fn, attrs, stack_alloc) );
        fibers_.push_back( f.get() );
        return f.release();
    }

    template< typename Fn, typename StackAllocator, typename Allocator >
    fiber * create_fiber( Fn fn, attributes attrs,
                          StackAllocator const& stack_alloc,
                          Allocator const& alloc)
    {
        mutex::scoped_lock lk( mtx_);

        std::auto_ptr< fiber > f( new fiber( fn, attrs, stack_alloc, alloc) );
        fibers_.push_back( f.get() );
        return f.release();
    }

    void add_fiber( fiber * f);

    void remove_fiber( fiber * f);

    void join_all();

    void interrupt_all();

    std::size_t size() const;

private:
    std::vector< fiber * >  fibers_;
    mutable mutex           mtx_;
};

}}

# if defined(BOOST_MSVC)
# pragma warning(pop)
# endif

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif


#endif // BOOST_FIBERS_FIBER_GROUP_H
