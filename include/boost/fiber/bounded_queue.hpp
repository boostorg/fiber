
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
//  idea of node-base locking from 'C++ Concurrency in Action', Anthony Williams

#ifndef BOOST_FIBERS_BOUNDED_QUEUE_H
#define BOOST_FIBERS_BOUNDED_QUEUE_H

#include <cstddef>
#include <stdexcept>
#include <utility>

#include <boost/atomic.hpp>
#include <boost/chrono/system_clocks.hpp>
#include <boost/config.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/move/move.hpp>
#include <boost/system/error_code.hpp>
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
struct bounded_queue_node
{
    typedef intrusive_ptr< bounded_queue_node >   ptr;

    std::size_t use_count;
    T           va;
    ptr         next;

    bounded_queue_node() :
        use_count( 0),
        va(),
        next()
    {}
};

template< typename T >
struct bounded_queue_node< T * >
{
    typedef intrusive_ptr< bounded_queue_node >   ptr;

    std::size_t use_count;
    T           va;
    ptr         next;

    bounded_queue_node() :
        use_count( 0),
        va( 0),
        next()
    {}
};

template< typename T >
void intrusive_ptr_add_ref( bounded_queue_node< T > * p)
{ ++p->use_count; }

template< typename T >
void intrusive_ptr_release( bounded_queue_node< T > * p)
{ if ( 0 == --p->use_count) delete p; }

}

template< typename T >
class bounded_queue : private noncopyable
{
public:
    typedef T   value_type;

private:
    typedef detail::bounded_queue_node< value_type >      node_type;  

    enum state_t
    {
        OPEN = 0,
        CLOSED
    };

    atomic< state_t >           state_;
    atomic< std::size_t >       count_;
    typename node_type::ptr     head_;
    mutable mutex               head_mtx_;
    typename node_type::ptr     tail_;
    mutable mutex               tail_mtx_;
    condition                   not_empty_cond_;
    condition                   not_full_cond_;
    std::size_t                 hwm_;
    std::size_t                 lwm_;

    bool is_closed_() const
    { return CLOSED == state_; }

    void close_()
    {
        state_ = CLOSED;
        not_empty_cond_.notify_all();
        not_full_cond_.notify_all();
    }

    std::size_t size_() const
    { return count_; }

    bool is_empty_() const
    { return head_ == get_tail_(); }

    bool is_full_() const
    { return count_ >= hwm_; }

    typename node_type::ptr get_tail_() const
    {
        mutex::scoped_lock lk( tail_mtx_);
        typename node_type::ptr tmp = tail_;
        return tmp;
    }

    typename node_type::ptr pop_head_()
    {
        typename node_type::ptr old_head = head_;
        head_ = old_head->next;
        --count_;
        return old_head;
    }

public:
    bounded_queue(
            std::size_t hwm,
            std::size_t lwm) :
        state_( OPEN),
        count_( 0),
        head_( new node_type() ),
        head_mtx_(),
        tail_( head_),
        tail_mtx_(),
        not_empty_cond_(),
        not_full_cond_(),
        hwm_( hwm),
        lwm_( lwm)
    {
        if ( hwm_ < lwm_)
            BOOST_THROW_EXCEPTION(
                invalid_argument(
                    system::errc::invalid_argument,
                    "boost fiber: high-watermark is less than low-watermark for bounded_queue") );
    }

    bounded_queue( std::size_t wm) :
        state_( OPEN),
        count_( 0),
        head_( new node_type() ),
        head_mtx_(),
        tail_( head_),
        tail_mtx_(),
        not_empty_cond_(),
        not_full_cond_(),
        hwm_( wm),
        lwm_( wm)
    {}

    std::size_t upper_bound() const
    { return hwm_; }

    std::size_t lower_bound() const
    { return lwm_; }

    void close()
    {
        mutex::scoped_lock head_lk( head_mtx_);
        mutex::scoped_lock tail_lk( tail_mtx_);
        close_();
    }

    bool is_closed() const
    { return is_closed_(); }

    bool is_empty() const
    { return is_empty_(); }

    bool is_full() const
    { return is_full_(); }

    queue_op_status push( value_type const& va)
    {
        typename node_type::ptr new_node( new node_type() );
        mutex::scoped_lock lk( tail_mtx_);

        if ( is_closed_() ) return queue_op_status::closed;

        while ( is_full_() ) not_full_cond_.wait( lk);

        tail_->va = va;
        tail_->next = new_node;
        tail_ = new_node;
        ++count_;
        not_empty_cond_.notify_one();

        return queue_op_status::success;
    }

    queue_op_status push( BOOST_RV_REF( value_type) va)
    {
        typename node_type::ptr new_node( new node_type() );
        mutex::scoped_lock lk( tail_mtx_);

        if ( is_closed_() ) return queue_op_status::closed;

        while ( is_full_() ) not_full_cond_.wait( lk);

        tail_->va = boost::move( va);
        tail_->next = new_node;
        tail_ = new_node;
        ++count_;
        not_empty_cond_.notify_one();

        return queue_op_status::success;
    }

    template< typename Rep, typename Period >
    queue_op_status push_wait_for( value_type const& va,
                                   chrono::duration< Rep, Period > const& timeout_duration)
    { return push_wait_until( va, clock_type::now() + timeout_duration); }

    template< typename Rep, typename Period >
    queue_op_status push_wait_for( BOOST_RV_REF( value_type) va,
                                   chrono::duration< Rep, Period > const& timeout_duration)
    { return push_wait_until( boost::move( va), clock_type::now() + timeout_duration); }

    queue_op_status push_wait_until( value_type const& va,
                                     clock_type::time_point const& timeout_time)
    {
        typename node_type::ptr new_node( new node_type() );
        mutex::scoped_lock lk( tail_mtx_);

        if ( is_closed_() ) return queue_op_status::closed;

        while ( is_full_() )
        {
            if ( cv_status::timeout == not_full_cond_.wait_until(
                                            lk, timeout_time) )
                return queue_op_status::timeout;
        }

        try
        {
            tail_->va = va;
            tail_->next = new_node;
            tail_ = new_node;
            ++count_;
            not_empty_cond_.notify_one();
            return queue_op_status::success;
        }
        catch (...)
        {
            close_();
            throw;
        }
    }

    queue_op_status push_wait_until( BOOST_RV_REF( value_type) va,
                                     clock_type::time_point const& timeout_time)
    {
        typename node_type::ptr new_node( new node_type() );
        mutex::scoped_lock lk( tail_mtx_);

        if ( is_closed_() ) return queue_op_status::closed;

        while ( is_full_() )
        {
            if ( cv_status::timeout == not_full_cond_.wait_until(
                                            lk, timeout_time) )
                return queue_op_status::timeout;
        }

        try
        {
            tail_->va = boost::move( va);
            tail_->next = new_node;
            tail_ = new_node;
            ++count_;
            not_empty_cond_.notify_one();
            return queue_op_status::success;
        }
        catch (...)
        {
            close_();
            throw;
        }
    }

    queue_op_status try_push( value_type const& va)
    {
        typename node_type::ptr new_node( new node_type() );
        mutex::scoped_lock lk( head_mtx_);

        if ( is_closed_() ) return queue_op_status::closed;
        if ( is_full_() ) return queue_op_status::full;

        try
        {
            tail_->va = va;
            tail_->next = new_node;
            tail_ = new_node;
            ++count_;
            not_empty_cond_.notify_one();
            return queue_op_status::success;
        }
        catch (...)
        {
            close_();
            throw;
        }
    }

    queue_op_status try_push( BOOST_RV_REF( value_type) va)
    {
        typename node_type::ptr new_node( new node_type() );
        mutex::scoped_lock lk( head_mtx_);

        if ( is_closed_() ) return queue_op_status::closed;
        if ( is_full_() ) return queue_op_status::full;

        try
        {
            tail_->va = boost::move( va);
            tail_->next = new_node;
            tail_ = new_node;
            ++count_;
            not_empty_cond_.notify_one();
            return queue_op_status::success;
        }
        catch (...)
        {
            close_();
            throw;
        }
    }

    queue_op_status pop( value_type & va)
    {
        mutex::scoped_lock lk( head_mtx_);

        while ( is_closed_() && is_empty_() ) not_empty_cond_.wait( lk);

        if ( is_closed_() && is_empty_() ) return queue_op_status::closed;
        BOOST_ASSERT( ! is_empty_() );

        try
        {
            va = boost::move( head_->va);
            pop_head_();
            if ( size_() <= lwm_)
            {
                if ( lwm_ == hwm_)
                    not_full_cond_.notify_one();
                else
                    // more than one producer could be waiting
                    // for submiting an action object
                    not_full_cond_.notify_all();
            }
            return queue_op_status::success;
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
    { return pop_wait_until( va, clock_type::now() + timeout_duration); }

    queue_op_status pop_wait_until( value_type & va,
                                    clock_type::time_point const& timeout_time)
    {
        mutex::scoped_lock lk( head_mtx_);

        while ( ! is_closed_() && is_empty_() )
        {
            if ( cv_status::timeout == not_empty_cond_.wait_until(
                                            lk, timeout_time) )
                return queue_op_status::timeout;
        }

        if ( is_closed_() && is_empty_() ) return queue_op_status::closed;
        BOOST_ASSERT( ! is_empty_() );

        try
        {
            va = boost::move( head_->va);
            pop_head_();
            if ( size_() <= lwm_)
            {
                if ( lwm_ == hwm_)
                    not_full_cond_.notify_one();
                else
                    // more than one producer could be waiting
                    // for submiting an action object
                    not_full_cond_.notify_all();
            }
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
        mutex::scoped_lock lk( head_mtx_);

        if ( is_closed_() && is_empty_() ) return queue_op_status::closed;
        if ( is_empty_() ) return queue_op_status::empty;

        va = boost::move( head_->va);
        pop_head_();
        if ( size_() <= lwm_)
        {
            if ( lwm_ == hwm_)
                not_full_cond_.notify_one();
            else
                // more than one producer could be waiting
                // in order to submit an task
                not_full_cond_.notify_all();
        }
        return queue_op_status::success;
    }
};

template< typename T >
class bounded_queue< T * > : private noncopyable
{
public:
    typedef T   *   value_type;

private:
    typedef detail::bounded_queue_node< value_type >      node_type;

    enum state_t
    {
        OPEN = 0,
        CLOSED
    };

    atomic< state_t >           state_;
    atomic< std::size_t >       count_;
    typename node_type::ptr     head_;
    mutable mutex               head_mtx_;
    typename node_type::ptr     tail_;
    mutable mutex               tail_mtx_;
    condition                   not_empty_cond_;
    condition                   not_full_cond_;
    std::size_t                 hwm_;
    std::size_t                 lwm_;

    bool is_closed_() const
    { return CLOSED == state_; }

    void close_()
    {
        state_ = CLOSED;
        not_empty_cond_.notify_all();
        not_full_cond_.notify_all();
    }

    std::size_t size_() const
    { return count_; }

    bool is_empty_() const
    { return head_ == get_tail_(); }

    bool is_full_() const
    { return count_ >= hwm_; }

    typename node_type::ptr get_tail_() const
    {
        mutex::scoped_lock lk( tail_mtx_);
        typename node_type::ptr tmp = tail_;
        return tmp;
    }

    typename node_type::ptr pop_head_()
    {
        typename node_type::ptr old_head = head_;
        head_ = old_head->next;
        --count_;
        return old_head;
    }

public:
    bounded_queue(
            std::size_t hwm,
            std::size_t lwm) :
        state_( OPEN),
        count_( 0),
        head_( new node_type() ),
        head_mtx_(),
        tail_( head_),
        tail_mtx_(),
        not_empty_cond_(),
        not_full_cond_(),
        hwm_( hwm),
        lwm_( lwm)
    {
        if ( hwm_ < lwm_)
            BOOST_THROW_EXCEPTION(
                invalid_argument(
                    system::errc::invalid_argument,
                    "boost fiber: high-watermark is less than low-watermark for bounded_queue") );
    }

    bounded_queue( std::size_t wm) :
        state_( OPEN),
        count_( 0),
        head_( new node_type() ),
        head_mtx_(),
        tail_( head_),
        tail_mtx_(),
        not_empty_cond_(),
        not_full_cond_(),
        hwm_( wm),
        lwm_( wm)
    {}

    std::size_t upper_bound() const
    { return hwm_; }

    std::size_t lower_bound() const
    { return lwm_; }

    void close()
    {
        mutex::scoped_lock head_lk( head_mtx_);
        mutex::scoped_lock tail_lk( tail_mtx_);
        close_();
    }

    bool is_closed() const
    { return is_closed_(); }

    bool is_empty() const
    { return is_empty_(); }

    bool is_full() const
    { return is_full_(); }

    queue_op_status push( value_type va)
    {
        typename node_type::ptr new_node( new node_type() );
        mutex::scoped_lock lk( tail_mtx_);

        if ( is_closed_() ) return queue_op_status::closed;

        while ( is_full_() ) not_full_cond_.wait( lk);

        tail_->va = va;
        tail_->next = new_node;
        tail_ = new_node;
        ++count_;
        not_empty_cond_.notify_one();

        return queue_op_status::success;
    }

    template< typename Rep, typename Period >
    queue_op_status push_wait_for( value_type va,
                                   chrono::duration< Rep, Period > const& timeout_duration)
    { return push_wait_until( va, clock_type::now() + timeout_duration); }

    queue_op_status push_wait_until( value_type va,
                                     clock_type::time_point const& timeout_time)
    {
        typename node_type::ptr new_node( new node_type() );
        mutex::scoped_lock lk( tail_mtx_);

        if ( is_closed_() ) return queue_op_status::closed;

        while ( is_full_() )
        {
            if ( cv_status::timeout == not_full_cond_.wait_until(
                                            lk, timeout_time) )
                return queue_op_status::timeout;
        }

        try
        {
            tail_->va = va;
            tail_->next = new_node;
            tail_ = new_node;
            ++count_;
            not_empty_cond_.notify_one();
            return queue_op_status::success;
        }
        catch (...)
        {
            close_();
            throw;
        }
    }

    queue_op_status try_push( value_type va)
    {
        typename node_type::ptr new_node( new node_type() );
        mutex::scoped_lock lk( head_mtx_);

        if ( is_closed_() ) return queue_op_status::closed;
        if ( is_full_() ) return queue_op_status::full;

        try
        {
            tail_->va = va;
            tail_->next = new_node;
            tail_ = new_node;
            ++count_;
            not_empty_cond_.notify_one();
            return queue_op_status::success;
        }
        catch (...)
        {
            close_();
            throw;
        }
    }

    queue_op_status pop( value_type va)
    {
        mutex::scoped_lock lk( head_mtx_);

        while ( is_closed_() && is_empty_() ) not_empty_cond_.wait( lk);

        if ( is_closed_() && is_empty_() ) return queue_op_status::closed;
        BOOST_ASSERT( ! is_empty_() );

        try
        {
            std::swap( va, head_->va);
            pop_head_();
            if ( size_() <= lwm_)
            {
                if ( lwm_ == hwm_)
                    not_full_cond_.notify_one();
                else
                    // more than one producer could be waiting
                    // for submiting an action object
                    not_full_cond_.notify_all();
            }
            return queue_op_status::success;
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
    { return pop_wait_until( va, clock_type::now() + timeout_duration); }

    queue_op_status pop_wait_until( value_type va,
                                    clock_type::time_point const& timeout_time)
    {
        mutex::scoped_lock lk( head_mtx_);

        while ( ! is_closed_() && is_empty_() )
        {
            if ( cv_status::timeout == not_empty_cond_.wait_until(
                                            lk, timeout_time) )
                return queue_op_status::timeout;
        }

        if ( is_closed_() && is_empty_() ) return queue_op_status::closed;
        BOOST_ASSERT( ! is_empty_() );

        try
        {
            std::swap( va, head_->va);
            pop_head_();
            if ( size_() <= lwm_)
            {
                if ( lwm_ == hwm_)
                    not_full_cond_.notify_one();
                else
                    // more than one producer could be waiting
                    // for submiting an action object
                    not_full_cond_.notify_all();
            }
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
        mutex::scoped_lock lk( head_mtx_);

        if ( is_closed_() && is_empty_() ) return queue_op_status::closed;
        if ( is_empty_() ) return queue_op_status::empty;

        std::swap( va, head_->va);
        pop_head_();
        if ( size_() <= lwm_)
        {
            if ( lwm_ == hwm_)
                not_full_cond_.notify_one();
            else
                // more than one producer could be waiting
                // in order to submit an task
                not_full_cond_.notify_all();
        }
        return queue_op_status::success;
    }
};

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_BOUNDED_QUEUE_H
