
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_PP_LIMIT_MAG  10

#include <cstdio>
#include <deque>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include <boost/assert.hpp>
#include <boost/atomic.hpp>
#include <boost/bind.hpp>
#include <boost/fiber/detail/spinlock.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/preprocessor/repetition/repeat_from_to.hpp>
#include <boost/thread.hpp>
#include <boost/utility.hpp>

#include "zeit.hpp"

class spinlock : private boost::noncopyable
{
private:
    enum state_t {
	    LOCKED = 0,
	    UNLOCKED
    };

    boost::atomic< state_t >           state_;

public:
    spinlock() :
        state_( UNLOCKED)
    {}

    void lock()
    {
        while ( LOCKED == state_.exchange( LOCKED) )
        {
            // busy-wait
            boost::this_thread::yield();
        }
    }

    void unlock()
    {
        BOOST_ASSERT( LOCKED == state_);

        state_ = UNLOCKED;
    }
};

template< typename T >
class spinlock_queue
{
private:
    typedef std::deque< T >     queue_t;

    spinlock     splk_;
    queue_t      queue_;

public:
    spinlock_queue() :
        splk_(),
        queue_()
    {}

    void push( T const& t)
    {
        boost::unique_lock< spinlock > lk( splk_);
        queue_.push_back( t);
    }

    bool pop( T & t)
    {
        boost::unique_lock< spinlock > lk( splk_);
        if ( queue_.empty() ) return false;
        std::swap( t, queue_.front() );
        queue_.pop_front();
        return true;
    }
};

boost::atomic_int producer_count(0);
boost::atomic_int consumer_count(0);

spinlock_queue< int > queue;

const int iterations = 500000;
const int producer_thread_count = 4;
const int consumer_thread_count = 4;

void producer(void)
{
    for (int i = 0; i != iterations; ++i) {
        int value = ++producer_count;
        queue.push(value);
    }
}

boost::atomic<bool> done (false);
void consumer(void)
{
    int value;
    while (!done) {
        while (queue.pop(value))
            ++consumer_count;
    }

    while (queue.pop(value))
        ++consumer_count;
}

zeit_t test_zeit( zeit_t ov)
{
    std::cout << "spinlock_queue is ";

    boost::thread_group producer_threads, consumer_threads;

    // start measurement
    zeit_t start( zeit() );

    for (int i = 0; i != producer_thread_count; ++i)
        producer_threads.create_thread(producer);

    for (int i = 0; i != consumer_thread_count; ++i)
        consumer_threads.create_thread(consumer);

    producer_threads.join_all();
    done = true;
    consumer_threads.join_all();

    // stop measurement
    zeit_t total( zeit() - start);

    std::cout << "produced " << producer_count << " objects." << std::endl;
    std::cout << "consumed " << consumer_count << " objects." << std::endl;

    // we have two jumps and two measuremt-overheads
    total -= ov; // overhead of measurement

    return total;
}

int main( int argc, char * argv[])
{
    try
    {
        zeit_t ov( overhead_zeit() );
        std::cout << "\noverhead for clock_gettime()  == " << ov << " ns" << std::endl;

        unsigned int res = test_zeit( ov);
        std::cout << "lockfree::queue< int >: " << (res/1000000) << " ms" << std::endl;

        return EXIT_SUCCESS;
    }
    catch ( std::exception const& e)
    { std::cerr << "exception: " << e.what() << std::endl; }
    catch (...)
    { std::cerr << "unhandled exception" << std::endl; }
    return EXIT_FAILURE;
}
