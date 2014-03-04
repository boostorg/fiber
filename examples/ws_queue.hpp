
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef WS_QUEUE_H
#define WS_QUEUE_H

#include <deque>

#include <boost/config.hpp>
#include <boost/thread/lock_types.hpp> 

#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/fiber_base.hpp>
#include <boost/fiber/detail/spinlock.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

class ws_queue
{
private:
    typedef std::deque< boost::fibers::detail::fiber_base::ptr_t >     queue_t;

    boost::fibers::detail::spinlock    splk_;
    queue_t     queue_;

public:
    ws_queue() :
        splk_(),
        queue_()
    {}

    void push( boost::fibers::detail::fiber_base::ptr_t const& f)
    {
        boost::unique_lock< boost::fibers::detail::spinlock > lk( splk_);
        queue_.push_back( f);
    }

    bool try_pop( boost::fibers::detail::fiber_base::ptr_t & f)
    {
        boost::unique_lock< boost::fibers::detail::spinlock > lk( splk_);
        queue_t::iterator e = queue_.end();
        for ( queue_t::iterator i = queue_.begin(); i != e; ++i)
        {
            if ( ! ( * i)->thread_affinity() )
            {
                f.swap( * i);
                queue_.erase( i);
                return true;
            }
        }
        return false;
    }
};

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif //  WS_QUEUE_H
