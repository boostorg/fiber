
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_FIBERS_UNBOUNDED_QUEUE_H
#define BOOST_FIBERS_UNBOUNDED_QUEUE_H

#include <cstddef>
#include <stdexcept>
#include <utility>

#include <boost/atomic.hpp>
#include <boost/chrono/system_clocks.hpp>
#include <boost/config.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/move/move.hpp>
#include <boost/thread/locks.hpp>
#include <boost/throw_exception.hpp>
#include <boost/utility.hpp>

#include <boost/fiber/exceptions.hpp>
#include <boost/fiber/condition.hpp>
#include <boost/fiber/mutex.hpp>
#include <boost/fiber/queue_op_status.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace detail {

template< typename T >
struct unbounded_queue_node
{
    typedef intrusive_ptr< unbounded_queue_node >   ptr;

    std::size_t use_count;
    T           va;
    ptr         next;

    unbounded_queue_node( T t) :
        use_count( 0),
        va( t),
        next()
    {}
};

template< typename T >
struct unbounded_queue_node< T * >
{
    typedef intrusive_ptr< unbounded_queue_node >   ptr;

    std::size_t use_count;
    T   *       va;
    ptr         next;

    unbounded_queue_node( T * t) :
        use_count( 0),
        va( t),
        next()
    {}
};

template< typename T >
void intrusive_ptr_add_ref( unbounded_queue_node< T > * p)
{ ++p->use_count; }

template< typename T >
void intrusive_ptr_release( unbounded_queue_node< T > * p)
{ if ( 0 == --p->use_count) delete p; }

}

template< typename T >
class unbounded_queue : private noncopyable
{
public:
    typedef T   value_type;

private:
    typedef detail::unbounded_queue_node< value_type >  node_type;

    enum state
    {
        OPEN = 0,
        CLOSED
    };

    state                       state_;
    typename node_type::ptr     head_;
    typename node_type::ptr     tail_;
    mutable mutex               mtx_;
    condition                   not_empty_cond_;

    bool is_closed_() const
    { return CLOSED == state_; }

    void close_()
    {
        state_ = CLOSED;
        not_empty_cond_.notify_all();
    }

    bool is_empty_() const
    { return 0 == head_; }

    typename node_type::ptr pop_head_()
    {
        typename node_type::ptr old_head = head_;
        head_ = old_head->next;
        if ( 0 == head_) tail_ = 0;
        old_head->next = 0;
        return old_head;
    }

    void push_tail_( typename node_type::ptr new_node)
    {
        if ( is_empty_() )
            head_ = tail_ = new_node;
        else
        {
            tail_->next = new_node;
            tail_ = new_node;
        }
    }

public:
    unbounded_queue() :
        state_( OPEN),
        head_(),
        tail_( head_),
        mtx_(),
        not_empty_cond_()
    {}

    void close()
    {
        boost::unique_lock< mutex > lk( mtx_);

        close_();
    }

    bool is_closed() const
    {
        boost::unique_lock< mutex > lk( mtx_);

        return is_closed_();
    }

    bool is_empty() const
    {
        boost::unique_lock< mutex > lk( mtx_);

        return is_empty_();
    }

    queue_op_status push( value_type const& va)
    {
        typename node_type::ptr new_node( new node_type( va) );
        boost::unique_lock< mutex > lk( mtx_);

        if ( is_closed_() ) return queue_op_status::closed;

        push_tail_( new_node);
        not_empty_cond_.notify_one();

        return queue_op_status::success;
    }

    queue_op_status push( BOOST_RV_REF( value_type) va)
    {
        typename node_type::ptr new_node( new node_type( boost::move( va) ) );
        boost::unique_lock< mutex > lk( mtx_);

        if ( is_closed_() ) return queue_op_status::closed;

        push_tail_( new_node);
        not_empty_cond_.notify_one();

        return queue_op_status::success;
    }

    queue_op_status pop( value_type & va)
    {
        boost::unique_lock< mutex > lk( mtx_);

        while ( ! is_closed_() && is_empty_() ) not_empty_cond_.wait( lk);

        if ( is_closed_() && is_empty_() ) return queue_op_status::closed;
        BOOST_ASSERT( ! is_empty_() );

        try
        {
            va = boost::move( head_->va);
            pop_head_();
            return queue_op_status::success;
        }
        catch (...)
        {
            close_();
            throw;
        }
    }

    value_type value_pop()
    {
        boost::unique_lock< mutex > lk( mtx_);

        while ( ! is_closed_() && is_empty_() ) not_empty_cond_.wait( lk);

        if ( is_closed_() && is_empty_() )
            BOOST_THROW_EXCEPTION(
                logic_error("boost fiber: queue is closed") );
        BOOST_ASSERT( ! is_empty_() );

        try
        {
            value_type va = boost::move( head_->va);
            pop_head_();
            return va;
        }
        catch (...)
        {
            close_();
            throw;
        }
    }

    template< typename Rep, typename Period >
    queue_op_status pop_wait_for( value_type & va,
                                  chrono::duration< Rep, Period > const& timeout_duration)
    { return pop_wait_until( va, chrono::high_resolution_clock::now() + timeout_duration); }

    template< typename TimePointType >
    queue_op_status pop_wait_until( value_type & va,
                                    TimePointType const& timeout_time)
    {
        boost::unique_lock< mutex > lk( mtx_);

        while ( ! is_closed_() && is_empty_() )
        {
            if ( cv_status::timeout == not_empty_cond_.wait_until( lk, timeout_time) )
                return queue_op_status::timeout;
        }

        if ( is_closed_() && is_empty_() ) return queue_op_status::closed;
        BOOST_ASSERT( ! is_empty_() );

        try
        {
            va = boost::move( head_->va);
            pop_head_();
            return queue_op_status::success;
        }
        catch (...)
        {
            close_();
            throw;
        }
    }

    queue_op_status try_pop( value_type & va)
    {
        boost::unique_lock< mutex > lk( mtx_);

        if ( is_closed_() && is_empty_() ) return queue_op_status::closed;
        if ( is_empty_() ) return queue_op_status::empty;

        va = boost::move( head_->va);
        pop_head_();
        return queue_op_status::success;
    }
};

template< typename T >
class unbounded_queue< T * > : private noncopyable
{
public:
    typedef T   *   value_type;

private:
    typedef detail::unbounded_queue_node< value_type >  node_type;

    enum state
    {
        OPEN = 0,
        CLOSED
    };

    state                       state_;
    typename node_type::ptr     head_;
    typename node_type::ptr     tail_;
    mutable mutex               mtx_;
    condition                   not_empty_cond_;

    bool is_closed_() const
    { return CLOSED == state_; }

    void close_()
    {
        state_ = CLOSED;
        not_empty_cond_.notify_all();
    }

    bool is_empty_() const
    { return 0 == head_; }

    typename node_type::ptr pop_head_()
    {
        typename node_type::ptr old_head = head_;
        head_ = old_head->next;
        if ( 0 == head_) tail_ = 0;
        old_head->next = 0;
        return old_head;
    }

    void push_tail_( typename node_type::ptr new_node)
    {
        if ( is_empty_() )
            head_ = tail_ = new_node;
        else
        {
            tail_->next = new_node;
            tail_ = new_node;
        }
    }

public:
    unbounded_queue() :
        state_( OPEN),
        head_(),
        tail_( head_),
        mtx_(),
        not_empty_cond_()
    {}

    void close()
    {
        boost::unique_lock< mutex > lk( mtx_);

        close_();
    }

    bool is_closed() const
    {
        boost::unique_lock< mutex > lk( mtx_);

        return is_closed_();
    }

    bool is_empty() const
    {
        boost::unique_lock< mutex > lk( mtx_);

        return is_empty_();
    }

    queue_op_status push( value_type va)
    {
        typename node_type::ptr new_node( new node_type( va) );
        boost::unique_lock< mutex > lk( mtx_);

        if ( is_closed_() ) return queue_op_status::closed;

        push_tail_( new_node);
        not_empty_cond_.notify_one();

        return queue_op_status::success;
    }

    queue_op_status pop( value_type va)
    {
        boost::unique_lock< mutex > lk( mtx_);

        while ( ! is_closed_() && is_empty_() ) not_empty_cond_.wait( lk);

        if ( is_closed_() && is_empty_() ) return queue_op_status::closed;
        BOOST_ASSERT( ! is_empty_() );

        try
        {
            std::swap( va, head_->va);
            pop_head_();
            return queue_op_status::success;
        }
        catch (...)
        {
            close_();
            throw;
        }
    }

    value_type value_pop()
    {
        boost::unique_lock< mutex > lk( mtx_);

        while ( ! is_closed_() && is_empty_() ) not_empty_cond_.wait( lk);

        if ( is_closed_() && is_empty_() )
            BOOST_THROW_EXCEPTION(
                logic_error("boost fiber: queue is closed") );
        BOOST_ASSERT( ! is_empty_() );

        try
        {
            value_type va = head_->va;
            pop_head_();
            return va;
        }
        catch (...)
        {
            close_();
            throw;
        }
    }

    template< typename Rep, typename Period >
    queue_op_status pop_wait_for( value_type va,
                                  chrono::duration< Rep, Period > const& timeout_duration)
    { return pop_wait_until( va, chrono::high_resolution_clock::now() + timeout_duration); }

    template< typename TimePointType >
    queue_op_status pop_wait_until( value_type va, TimePointType const& timeout_time)
    {
        boost::unique_lock< mutex > lk( mtx_);

        while ( ! is_closed_() && is_empty_() )
        {
            if ( cv_status::timeout == not_empty_cond_.wait_until( lk, timeout_time) )
                return queue_op_status::timeout;
        }

        if ( is_closed_() && is_empty_() ) return queue_op_status::closed;
        BOOST_ASSERT( ! is_empty_() );

        try
        {
            std::swap( va, head_->va);
            pop_head_();
            return queue_op_status::success;
        }
        catch (...)
        {
            close_();
            throw;
        }
    }

    queue_op_status try_pop( value_type va)
    {
        boost::unique_lock< mutex > lk( mtx_);

        if ( is_closed_() && is_empty_() ) return queue_op_status::closed;
        if ( is_empty_() ) return queue_op_status::empty;

        try
        {
            std::swap( va, head_->va);
            pop_head_();
            return queue_op_status::success;
        }
        catch (...)
        {
            close_();
            throw;
        }   
    }
};

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_UNBOUNDED_QUEUE_H
