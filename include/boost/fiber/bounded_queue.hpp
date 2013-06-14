
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
//  idea of node-base locking from 'C++ Concurrency in Action', Anthony Williams

#ifndef BOOST_FIBERS_BOUNDED_CHANNEL_H
#define BOOST_FIBERS_BOUNDED_CHANNEL_H

#include <cstddef>
#include <stdexcept>

#include <boost/chrono/system_clocks.hpp>
#include <boost/config.hpp>
#include <boost/exception/all.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/optional.hpp>
#include <boost/system/error_code.hpp>
#include <boost/utility.hpp>

#include <boost/fiber/exceptions.hpp>
#include <boost/fiber/condition.hpp>
#include <boost/fiber/mutex.hpp>

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
    typedef optional< T >   value_type;

private:
    typedef detail::bounded_queue_node< value_type >      node_type;  

    template< typename X >
    friend void intrusive_ptr_add_ref( bounded_queue< X > * p);
    template< typename X >
    friend void intrusive_ptr_release( bounded_queue< X > * p);

    enum state_t
    {
        ACTIVE = 0,
        DEACTIVE
    };

    state_t                     state_;
    std::size_t                 count_;
    typename node_type::ptr     head_;
    mutable mutex               head_mtx_;
    typename node_type::ptr     tail_;
    mutable mutex               tail_mtx_;
    condition                   not_empty_cond_;
    condition                   not_full_cond_;
    unsigned int                hwm_;
    unsigned int                lwm_;

    bool active_() const
    { return ACTIVE == state_; }

    void deactivate_()
    { state_ = DEACTIVE; }

    std::size_t size_() const
    { return count_; }

    bool empty_() const
    { return head_ == get_tail_(); }

    bool full_() const
    { return size_() >= hwm_; }

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
        state_( ACTIVE),
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
            boost::throw_exception(
                invalid_argument(
                    system::errc::invalid_argument,
                    "boost fiber: high-watermark is less than low-watermark for bounded_queue") );
    }

    bounded_queue( std::size_t wm) :
        state_( ACTIVE),
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

    bool active() const
    { return active_(); }

    void deactivate()
    {
        mutex::scoped_lock head_lk( head_mtx_);
        mutex::scoped_lock tail_lk( tail_mtx_);
        deactivate_();
        not_empty_cond_.notify_all();
        not_full_cond_.notify_all();
    }

    bool empty() const
    { return empty_(); }

    void put( T const& t)
    {
        typename node_type::ptr new_node( new node_type() );
        {
            mutex::scoped_lock lk( tail_mtx_);

            if ( full_() )
            {
                while ( active_() && full_() )
                    not_full_cond_.wait( lk);
            }

            if ( ! active_() )
                boost::throw_exception( fiber_resource_error() );

            tail_->va = t;
            tail_->next = new_node;
            tail_ = new_node;
            ++count_;
        }
        not_empty_cond_.notify_one();
    }
#if 0
    template< typename TimeDuration >
    bool put( T const& t, TimeDuration const& dt)
    { return put( t, chrono::system_clock::now() + dt); }

    bool put( T const& t, chrono::system_clock::time_point const& abs_time)
    {
        typename node_type::ptr new_node( new node_type() );
        {
            mutex::scoped_lock lk( tail_mtx_);

            if ( full_() )
            {
                while ( active_() && full_() )
                {
                    if ( ! not_full_cond_.timed_wait( lk, abs_time) )
                        return false;
                }
            }

            if ( ! active_() )
                boost::throw_exception( fiber_resource_error() );

            tail_->va = t;
            tail_->next = new_node;
            tail_ = new_node;
            ++count_;
        }
        not_empty_cond_.notify_one();
        return true;
    }
#endif
    bool take( value_type & va)
    {
        mutex::scoped_lock lk( head_mtx_);
        bool empty = empty_();
        if ( ! active_() && empty)
            return false;
        if ( empty)
        {
            try
            {
                while ( active_() && empty_() )
                    not_empty_cond_.wait( lk);
            }
            catch ( fiber_interrupted const&)
            { return false; }
        }
        if ( ! active_() && empty_() )
            return false;
        swap( va, head_->va);
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
        return va;
    }
#if 0
    template< typename TimeDuration >
    bool take( value_type & va, TimeDuration const& dt)
    { return take( va, chrono::system_clock::now() + dt); }

    bool take( value_type & va, chrono::system_clock::time_point const& abs_time)
    {
        mutex::scoped_lock lk( head_mtx_);
        bool empty = empty_();
        if ( ! active_() && empty)
            return false;
        if ( empty)
        {
            try
            {
                while ( active_() && empty_() )
                {
                    if ( ! not_empty_cond_.timed_wait( lk, abs_time) )
                        return false;
                }
            }
            catch ( fiber_interrupted const&)
            { return false; }
        }
        if ( ! active_() && empty_() )
            return false;
        swap( va, head_->va);
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
        return va;
    }
#endif
    bool try_take( value_type & va)
    {
        mutex::scoped_lock lk( head_mtx_);
        if ( empty_() )
            return false;
        swap( va, head_->va);
        pop_head_();
        bool valid = va;
        if ( valid && size_() <= lwm_)
        {
            if ( lwm_ == hwm_)
                not_full_cond_.notify_one();
            else
                // more than one producer could be waiting
                // in order to submit an task
                not_full_cond_.notify_all();
        }
        return valid;
    }
};

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_BOUNDED_CHANNEL_H
