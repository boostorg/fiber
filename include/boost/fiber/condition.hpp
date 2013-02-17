
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
//  based on boost::interprocess::sync::interprocess_condition

#ifndef BOOST_FIBERS_CONDITION_H
#define BOOST_FIBERS_CONDITION_H

#include <algorithm>
#include <cstddef>
#include <deque>

#include <boost/assert.hpp>
#include <boost/atomic.hpp>
#include <boost/chrono/system_clocks.hpp>
#include <boost/config.hpp>
#include <boost/thread/locks.hpp>
#include <boost/utility.hpp>

#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/notify.hpp>
#include <boost/fiber/detail/scheduler.hpp>
#include <boost/fiber/detail/spinlock.hpp>
#include <boost/fiber/exceptions.hpp>
#include <boost/fiber/interruption.hpp>
#include <boost/fiber/mutex.hpp>
#include <boost/fiber/operations.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

# if defined(BOOST_MSVC)
# pragma warning(push)
# pragma warning(disable:4355 4251 4275)
# endif

#include <cstdio>

namespace boost {
namespace fibers {

class BOOST_FIBERS_DECL condition : private noncopyable
{
private:
    detail::spinlock                       waiting_mtx_;
    std::deque< detail::notify::ptr_t >    waiting_;

public:
    condition();

    ~condition();

    void notify_one();

    void notify_all();

    template< typename LockType, typename Pred >
    void wait( LockType & lt, Pred pred)
    {
        while ( ! pred() )
            wait( lt);
    }

    template< typename LockType >
    void wait( LockType & lt)
    {
        detail::notify::ptr_t n( detail::scheduler::instance().active() );
        try
        {
            if ( n)
            {
                // store this fiber in order to be notified later
                unique_lock< detail::spinlock > lk( waiting_mtx_);
                waiting_.push_back( n);
                lt.unlock();

                // suspend fiber
                detail::scheduler::instance().wait( lk);

                // check if fiber was interrupted
                this_fiber::interruption_point();
            }
            else
            {
                // notifier for main-fiber
                n = detail::scheduler::instance().notifier();
                // store this fiber in order to be notified later
                unique_lock< detail::spinlock > lk( waiting_mtx_);
                waiting_.push_back( n);
                lt.unlock();

                lk.unlock();
                while ( ! n->is_ready() )
                {
                    fprintf(stdout, "condition: main-fiber not woken-up\n");
                    // run scheduler
                    detail::scheduler::instance().run();
                }
                    fprintf(stdout, "condition: main-fiber woken-up\n");
            }
        }
        catch (...)
        {
            // remove fiber from waiting_
            unique_lock< detail::spinlock > lk( waiting_mtx_);
            waiting_.erase(
                std::find( waiting_.begin(), waiting_.end(), n) );
            throw;
        }

        // lock external again before returning
        lt.lock();
    }
};

typedef condition condition_variable;
typedef condition condition_variable_any;

}}

# if defined(BOOST_MSVC)
# pragma warning(pop)
# endif

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_CONDITION_H
