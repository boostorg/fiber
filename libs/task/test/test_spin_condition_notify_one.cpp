// Copyright (C) 2007 Anthony Williams
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/bind.hpp>
#include <boost/thread.hpp>

#include <boost/test/unit_test.hpp>

#include <boost/task.hpp>

#include <libs/task/test/util.ipp>

#include "condition_test_common.hpp"

namespace tsk = boost::tasks;

void do_test_condition_notify_one_wakes_from_wait()
{
    wait_for_flag data;
    
    boost::thread thread(boost::bind(&wait_for_flag::wait_without_predicate, boost::ref(data)));

    {
        tsk::spin::mutex::scoped_lock lock(data.mutex);
        data.flag=true;
        data.cond_var.notify_one();
    }

    thread.join();
    BOOST_CHECK(data.woken);
}

void do_test_condition_notify_one_wakes_from_wait_with_predicate()
{
    wait_for_flag data;
    
    boost::thread thread(boost::bind(&wait_for_flag::wait_with_predicate, boost::ref(data)));

    {
        tsk::spin::mutex::scoped_lock lock(data.mutex);
        data.flag=true;
        data.cond_var.notify_one();
    }

    thread.join();
    BOOST_CHECK(data.woken);
}

void do_test_condition_notify_one_wakes_from_timed_wait()
{
    wait_for_flag data;
    
    boost::thread thread(boost::bind(&wait_for_flag::timed_wait_without_predicate, boost::ref(data)));

    {
        tsk::spin::mutex::scoped_lock lock(data.mutex);
        data.flag=true;
        data.cond_var.notify_one();
    }

    thread.join();
    BOOST_CHECK(data.woken);
}

void do_test_condition_notify_one_wakes_from_timed_wait_with_predicate()
{
    wait_for_flag data;
    
    boost::thread thread(boost::bind(&wait_for_flag::timed_wait_with_predicate, boost::ref(data)));

    {
        tsk::spin::mutex::scoped_lock lock(data.mutex);
        data.flag=true;
        data.cond_var.notify_one();
    }

    thread.join();
    BOOST_CHECK(data.woken);
}

void do_test_condition_notify_one_wakes_from_relative_timed_wait_with_predicate()
{
    wait_for_flag data;
    
    boost::thread thread(boost::bind(&wait_for_flag::relative_timed_wait_with_predicate, boost::ref( data)));

    {
        tsk::spin::mutex::scoped_lock lock(data.mutex);
        data.flag=true;
        data.cond_var.notify_one();
    }

    thread.join();
    BOOST_CHECK(data.woken);
}

namespace
{
    tsk::spin::mutex multiple_wake_mutex;
    tsk::spin::condition multiple_wake_cond;
    unsigned multiple_wake_count=0;

    void wait_for_condvar_and_increase_count()
    {
        tsk::spin::mutex::scoped_lock lk(multiple_wake_mutex);
        multiple_wake_cond.wait(lk);
        ++multiple_wake_count;
    }
    
}


void do_test_multiple_notify_one_calls_wakes_multiple_threads()
{
    boost::thread thread1(wait_for_condvar_and_increase_count);
    boost::thread thread2(wait_for_condvar_and_increase_count);

    boost::this_thread::sleep(boost::posix_time::milliseconds(200));
    multiple_wake_cond.notify_one();

    boost::thread thread3(wait_for_condvar_and_increase_count);

    boost::this_thread::sleep(boost::posix_time::milliseconds(200));
    multiple_wake_cond.notify_one();
    multiple_wake_cond.notify_one();
    boost::this_thread::sleep(boost::posix_time::milliseconds(200));
    
    {
        tsk::spin::mutex::scoped_lock lk(multiple_wake_mutex);
        BOOST_CHECK(multiple_wake_count==3);
    }

    thread1.join();
    thread2.join();
    thread3.join();
}

void test_condition_notify_one()
{
    timed_test(&do_test_condition_notify_one_wakes_from_wait, timeout_seconds, execution_monitor::use_mutex);
    timed_test(&do_test_condition_notify_one_wakes_from_wait_with_predicate, timeout_seconds, execution_monitor::use_mutex);
    timed_test(&do_test_condition_notify_one_wakes_from_timed_wait, timeout_seconds, execution_monitor::use_mutex);
    timed_test(&do_test_condition_notify_one_wakes_from_timed_wait_with_predicate, timeout_seconds, execution_monitor::use_mutex);
    timed_test(&do_test_condition_notify_one_wakes_from_relative_timed_wait_with_predicate, timeout_seconds, execution_monitor::use_mutex);
    timed_test(&do_test_multiple_notify_one_calls_wakes_multiple_threads, timeout_seconds, execution_monitor::use_mutex);
}


boost::unit_test_framework::test_suite* init_unit_test_suite(int, char*[])
{
    boost::unit_test_framework::test_suite* test =
        BOOST_TEST_SUITE("Boost.Task: condition test suite");

    test->add(BOOST_TEST_CASE(&test_condition_notify_one));

    return test;
}
