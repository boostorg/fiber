
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// This test is based on the tests of Boost.Thread 

#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <vector>

#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/function.hpp>
#include <boost/ref.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/thread.hpp>
#include <boost/utility.hpp>

#include <boost/task.hpp>

#include <libs/task/test/util.ipp>

namespace pt = boost::posix_time;
namespace tsk = boost::tasks;

template< typename M >
struct test_lock
{
    typedef M mutex_type;
    typedef typename M::scoped_lock lock_type;

    void operator()()
    {
        mutex_type mutex;
        tsk::spin::condition condition;

        // Test the lock's constructors.
        {
            lock_type lock(mutex, boost::defer_lock);
            BOOST_CHECK(!lock);
        }
        lock_type lock(mutex);
        BOOST_CHECK(lock ? true : false);

        // Construct and initialize an xtime for a fast time out.
        pt::time_duration xt = pt::milliseconds( 100);

        // Test the lock and the mutex with condition variables.
        // No one is going to notify this condition variable.  We expect to
        // time out.
        BOOST_CHECK(!condition.timed_wait(lock, xt));
        BOOST_CHECK(lock ? true : false);

        // Test the lock and unlock methods.
        lock.unlock();
        BOOST_CHECK(!lock);
        lock.lock();
        BOOST_CHECK(lock ? true : false);
    }
};

void do_test_mutex()
{
    test_lock< tsk::spin::mutex >()();
}

void test_mutex()
{
    timed_test(&do_test_mutex, 3);
}


boost::unit_test::test_suite * init_unit_test_suite( int, char* [])
{
    boost::unit_test_framework::test_suite * test =
		BOOST_TEST_SUITE("Boost.Task: spin-mutex test suite");

    test->add(BOOST_TEST_CASE(&test_mutex));

	return test;
}
