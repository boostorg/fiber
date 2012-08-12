
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_DETAIL_STRATUM_BASE_H
#define BOOST_FIBERS_DETAIL_STRATUM_BASE_H

#include <cstddef>
#include <iostream>
#include <vector>

#include <boost/assert.hpp>
#include <boost/chrono/system_clocks.hpp>
#include <boost/config.hpp>
#include <boost/context/fcontext.hpp>
#include <boost/context/stack_allocator.hpp>
#include <boost/cstdint.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/utility.hpp>

#include <boost/fiber/detail/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace detail {

struct forced_unwind {};

enum flag_t
{
    flag_resumed                = 1 << 1,
    flag_complete               = 1 << 2,
    flag_unwind_stack           = 1 << 3,
    flag_canceled               = 1 << 4
};

template< typename Fiber >
void trampoline( intptr_t vp)
{
    BOOST_ASSERT( vp);

    Fiber * f( reinterpret_cast< Fiber * >( vp) );
    BOOST_ASSERT( ! f->is_complete() );
    BOOST_ASSERT( f->is_resumed() );
    try
    { f->exec(); }
    catch ( forced_unwind const&)
    {}
    catch (...)
    {
        //TODO: call std::terminate() or do nothing?
    }
    f->flags_ &= ~flag_resumed;
    f->flags_ &= ~flag_unwind_stack;
    f->flags_ |= flag_complete;

    ctx::jump_fcontext( & f->callee_, & f->caller_, 0, f->preserve_fpu_);
}

class BOOST_FIBERS_DECL fiber_base : private noncopyable
{
public:
    typedef intrusive_ptr< fiber_base >           ptr_t;

private:
    template< typename T >
    friend void trampoline( intptr_t);
    friend class scheduler;

    std::size_t             use_count_;
    ctx::stack_allocator    alloc_;
    ctx::fcontext_t         caller_;
    ctx::fcontext_t         callee_;
    int                     flags_;
    bool                    preserve_fpu_;
    std::vector< ptr_t >    joining_;

    void unwind_stack_();

    void terminate_();

    void notify_();

public:
    class id
    {
    private:
        friend class fiber_base;

        fiber_base::ptr_t   impl_;

        id( fiber_base::ptr_t const& impl) :
            impl_( impl)
        {}

    public:
        id() :
            impl_()
        {}

        bool operator==( id const& other) const
        { return impl_ == other.impl_; }

        bool operator!=( id const& other) const
        { return impl_ != other.impl_; }
        
        bool operator<( id const& other) const
        { return impl_ < other.impl_; }
        
        bool operator>( id const& other) const
        { return other.impl_ < impl_; }
        
        bool operator<=( id const& other) const
        { return ! ( * this > other); }
        
        bool operator>=( id const& other) const
        { return ! ( * this < other); }

        template< typename charT, class traitsT >
        friend std::basic_ostream< charT, traitsT > &
        operator<<( std::basic_ostream< charT, traitsT > & os, id const& other)
        {
            if ( 0 != other.impl_)
                return os << other.impl_;
            else
                return os << "{not-valid}";
        }

        operator bool() const
        { return 0 != impl_; }

        bool operator!() const
        { return 0 == impl_; }
    };

    fiber_base( std::size_t size, bool preserve_fpu);

    virtual ~fiber_base();

    id get_id() const;

    bool is_canceled() const;

    bool is_complete() const;

    bool is_resumed() const;

    void join( ptr_t const& p);

    void resume();

    void suspend();

    void cancel();

    void notify();

    void wait();

    void sleep( chrono::system_clock::time_point const& abs_time);

    virtual void exec() = 0;

    friend inline void intrusive_ptr_add_ref( fiber_base * p)
    { ++p->use_count_; }

    friend inline void intrusive_ptr_release( fiber_base * p)
    { if ( --p->use_count_ == 0) delete p; }
};

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DETAIL_STRATUM_BASE_H
