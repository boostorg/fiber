
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

//  node-base locking based on 'C++ Concurrency in Action', Anthony Williams

#ifndef BOOST_FIBERS_DETAIL_QUEUE_H
#define BOOST_FIBERS_DETAIL_QUEUE_H

#include <cstddef>

#include <boost/config.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/utility.hpp>

#include <boost/fiber/detail/fiber_base.hpp>
#include <boost/fiber/detail/spin_mutex.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace detail {

struct node
{
    typedef intrusive_ptr< node >   ptr_t;

    std::size_t         use_count;
    fiber_base::ptr_t   f;
    ptr                 next;

    node() :
        use_count( 0),
        f(),
        next()
    {}
};

inline
void intrusive_ptr_add_ref( node * p)
{ ++p->use_count; }

inline
void intrusive_ptr_release( node * p)
{ if ( 0 == --p->use_count) delete p; }

class queue : private noncopyable
{
private:
    node::ptr_t             head_;
    mutable spin_mutex      head_mtx_;
    node::ptr_t             tail_;
    mutable spin_mutex      tail_mtx_;

    bool empty_() const
    { return head_ == get_tail_(); }

    node_type::ptr get_tail_() const
    {
        spin_mutex::scoped_lock lk( tail_mtx_); 
        node::ptr_t tmp = tail_;
        return tmp;
    }

    node::ptr_t pop_head_()
    {
        node_type::ptr_t old_head = head_;
        head_ = old_head->next;
        return old_head;
    }

public:
    queue() :
        head_( new node_type() ),
        head_mtx_(),
        tail_( head_),
        tail_mtx_()
    {}

    bool empty() const
    {
        spin_mutex::scoped_lock lk( head_mtx_);
        return empty_();
    }

    void push( fiber_base::ptr_t const& f)
    {
        node::ptr_t new_node( new node_type() );
        {
            spin_mutex::scoped_lock lk( tail_mtx_);
            tail_->f = f;
            tail_->next = new_node;
            tail_ = new_node;
        }
    }

    void notify_one()
    {
        spin_mutex::scoped_lock lk( head_mtx_);
        while ( ! empty_() )
        {
            BOOST_ASSERT( head_->f);
            bool result = head_->f->set_ready();
            pop_head_();
            if ( result) break;
        }
    }

    void notify_all()
    {
        spin_mutex::scoped_lock lk( head_mtx_);
        while ( ! empty_() )
        {
            BOOST_ASSERT( head_->f);
            head_->f->set_ready();
            pop_head_();
        }
    }
};

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DETAIL_QUEUE_H
