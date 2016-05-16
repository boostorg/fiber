
//          Copyright Oliver Kowalke 2015.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <deque>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

#include <boost/assert.hpp>

#include <boost/fiber/all.hpp>

#include "barrier.hpp"

static std::atomic< bool > fini{ false };

boost::fibers::future< int > fibonacci( int);

int fibonacci_( int n) {
    boost::this_fiber::yield();
    if ( 0 == n) {
        return 0;
    } else if ( 1 == n || 2 == n) {
        return 1;
    } else { 
        boost::fibers::future< int > f1 = fibonacci( n - 1);
        boost::fibers::future< int > f2 = fibonacci( n - 2);
        return f1.get() + f2.get();
    }
}

boost::fibers::future< int > fibonacci( int n) {
    boost::fibers::packaged_task< int() > pt( std::bind( fibonacci_, n) );
    boost::fibers::future< int > f( pt.get_future() );
    boost::fibers::fiber( boost::fibers::launch::dispatch, std::move( pt) ).detach();
    return f;
}

void thread( unsigned int max_idx, unsigned int idx, barrier * b) {
    boost::fibers::use_scheduling_algorithm< boost::fibers::algo::random_chase_lev >( max_idx, idx);
    b->wait();
    while ( ! fini) {
        // To guarantee progress, we must ensure that
        // threads that have work to do are not unreasonably delayed by (thief) threads
        // which are idle except for task-stealing. 
        // This call yields the thief ’s processor to another thread, allowing
        // descheduled threads to regain a processor and make progress. 
        std::this_thread::yield();
        boost::this_fiber::yield();
    }
}

int main() {
    unsigned int max_idx = 8;
    boost::fibers::use_scheduling_algorithm< boost::fibers::algo::random_chase_lev >( max_idx, max_idx);
    std::vector< std::thread > threads;
    barrier b( max_idx + 1);
    // launch a couple threads to help process them
    for ( unsigned int idx = 0; idx < max_idx; ++idx) {
        threads.push_back( std::thread( thread, max_idx, idx, & b) );
    };
    b.wait();
    int n = 30;
    // main fiber computes fibonacci( n)
    // wait on result
    int result = fibonacci( n).get();
//    BOOST_ASSERT( 55 == result);
    std::ostringstream buffer;
    buffer << "fibonacci(" << n << ") = " << result << '\n';
    std::cout << buffer.str() << std::flush;
    // set termination flag
    fini = true;
    // wait for threads to terminate
    for ( std::thread & t : threads) {
        t.join();
    }
    std::cout << "done." << std::endl;
    return EXIT_SUCCESS;
}
