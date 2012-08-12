
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_FIBER_H
#define BOOST_FIBERS_FIBER_H

#include <cstddef>

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/context/stack_utils.hpp>
#include <boost/move/move.hpp>

#include <boost/fiber/detail/fiber_base.hpp>
#include <boost/fiber/detail/fiber_object.hpp>

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

class scheduler;

}

class fiber;
fiber migrate_from();
void migrate_to( BOOST_RV_REF( fiber) );

class BOOST_FIBERS_DECL fiber
{
private:
    friend fiber migrate_from();
    friend void migrate_to( BOOST_RV_REF( fiber) );
    friend class detail::scheduler;

    typedef detail::fiber_base    base_t;
    typedef base_t::ptr_t           ptr_t;

    ptr_t       impl_;

    BOOST_MOVABLE_BUT_NOT_COPYABLE( fiber);

    fiber( ptr_t const& impl) :
        impl_( impl)
    { BOOST_ASSERT( impl_); }

    template< typename Fn >
    fiber( Fn fn, std::size_t size = ctx::default_stacksize(),
             bool preserve_fpu = true) :
        impl_(
            new detail::fiber_object< Fn >(
                fn, size, preserve_fpu) )
    {}

    template< typename Fn >
    fiber( BOOST_RV_REF( Fn) fn, std::size_t size = ctx::default_stacksize(),
             bool preserve_fpu = true) :
        impl_(
            new detail::fiber_object< Fn >(
                boost::move( fn), size, preserve_fpu) )
    {}

public:
    typedef detail::fiber_base::id        id;
    typedef void ( * unspecified_bool_type)( fiber ***);

    static void unspecified_bool( fiber ***) {}

    fiber() :
        impl_()
    {}

    fiber( BOOST_RV_REF( fiber) other) :
        impl_()
    { impl_.swap( other.impl_); }

    fiber & operator=( BOOST_RV_REF( fiber) other)
    {
        fiber tmp( other);
        swap( tmp);
        return * this;
    }

    operator unspecified_bool_type() const;

    bool operator!() const;

    void swap( fiber & other);

    bool operator==( fiber const& other) const;
    bool operator!=( fiber const& other) const;

    id get_id() const;

    bool is_joinable() const;

    bool is_complete() const;

    void cancel();

    bool join();
};

inline
bool operator<( fiber const& l, fiber const& r)
{ return l.get_id() < r.get_id(); }

inline
void swap( fiber & l, fiber & r)
{ return l.swap( r); }

}}

# if defined(BOOST_MSVC)
# pragma warning(pop)
# endif

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_FIBER_H
