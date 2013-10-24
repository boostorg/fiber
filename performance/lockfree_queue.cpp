
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_PP_LIMIT_MAG  10

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include <boost/assert.hpp>
#include <boost/atomic.hpp>
#include <boost/bind.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/preprocessor/repetition/repeat_from_to.hpp>
#include <boost/thread/thread.hpp>

#include "zeit.hpp"

boost::atomic_int producer_count(0);
boost::atomic_int consumer_count(0);

boost::lockfree::queue< int > queue( 128);

const int iterations = 100000;
const int producer_thread_count = 4;
const int consumer_thread_count = 4;

void producer(void)
{
    for (int i = 0; i != iterations; ++i) {
        int value = ++producer_count;
        while (!queue.push(value))
            ;
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
    std::cout << "boost::lockfree::queue is ";
    if (!queue.is_lock_free())
        std::cout << "not ";
    std::cout << "lockfree" << std::endl;

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
