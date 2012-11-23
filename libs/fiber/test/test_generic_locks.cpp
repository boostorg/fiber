// (C) Copyright 2008 Anthony Williams
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/test/unit_test.hpp>
#include <boost/thread.hpp>

#include <boost/fiber/all.hpp>

namespace stm = boost::fibers;
namespace this_stm = boost::this_fiber;

struct wait_data
{
    stm::mutex m;
    bool flag;
    stm::condition cond;
    
    wait_data():
        flag(false)
    {}
    
    void wait()
    {
        stm::mutex::scoped_lock l(m);
        while(!flag)
        {
            cond.wait(l);
        }
    }
    
    void signal()
    {
        stm::mutex::scoped_lock l(m);
        flag=true;
        cond.notify_all();
    }
};
       
void lock_pair(stm::mutex* m1,stm::mutex* m2)
{
    boost::lock(*m1,*m2);
    stm::mutex::scoped_lock l1(*m1,boost::adopt_lock),
        l2(*m2,boost::adopt_lock);
}

void lock_five(stm::mutex* m1,stm::mutex* m2,stm::mutex* m3,stm::mutex* m4,stm::mutex* m5)
{
    boost::lock(*m1,*m2,*m3,*m4,*m5);
    m1->unlock();
    m2->unlock();
    m3->unlock();
    m4->unlock();
    m5->unlock();
}

void lock_n(stm::mutex* mutexes,unsigned count)
{
    boost::lock(mutexes,mutexes+count);
    for(unsigned i=0;i<count;++i)
    {
        mutexes[i].unlock();
    }
}

struct dummy_mutex
{
    bool is_locked;
    
    dummy_mutex():
        is_locked(false)
    {}
    
    void lock()
    {
        is_locked=true;
    }
    
    bool try_lock()
    {
        if(is_locked)
        {
            return false;
        }
        is_locked=true;
        return true;
    }
    
    void unlock()
    {
        is_locked=false;
    }
};

namespace boost
{
    template<>
    struct is_mutex_type<dummy_mutex>
    {
        BOOST_STATIC_CONSTANT(bool, value = true);
    };
}

void lock_two_uncontended()
{
    stm::mutex m1,m2;

    stm::mutex::scoped_lock l1(m1,boost::defer_lock),
        l2(m2,boost::defer_lock);

    BOOST_CHECK(!l1.owns_lock());
    BOOST_CHECK(!l2.owns_lock());
    
    boost::lock(l1,l2);
    
    BOOST_CHECK(l1.owns_lock());
    BOOST_CHECK(l2.owns_lock());
}

void lock_five_uncontended()
{
    stm::mutex m1,m2,m3,m4,m5;

    stm::mutex::scoped_lock l1(m1,boost::defer_lock),
        l2(m2,boost::defer_lock),
        l3(m3,boost::defer_lock),
        l4(m4,boost::defer_lock),
        l5(m5,boost::defer_lock);

    BOOST_CHECK(!l1.owns_lock());
    BOOST_CHECK(!l2.owns_lock());
    BOOST_CHECK(!l3.owns_lock());
    BOOST_CHECK(!l4.owns_lock());
    BOOST_CHECK(!l5.owns_lock());
    
    boost::lock(l1,l2,l3,l4,l5);
    
    BOOST_CHECK(l1.owns_lock());
    BOOST_CHECK(l2.owns_lock());
    BOOST_CHECK(l3.owns_lock());
    BOOST_CHECK(l4.owns_lock());
    BOOST_CHECK(l5.owns_lock());
}

void lock_five_in_range()
{
    unsigned const num_mutexes=5;
    dummy_mutex mutexes[num_mutexes];

    boost::lock(mutexes,mutexes+num_mutexes);
    
    for(unsigned i=0;i<num_mutexes;++i)
    {
        BOOST_CHECK(mutexes[i].is_locked);
    }
}

void lock_ten_in_range()
{
    unsigned const num_mutexes=10;
    dummy_mutex mutexes[num_mutexes];

    boost::lock(mutexes,mutexes+num_mutexes);
    
    for(unsigned i=0;i<num_mutexes;++i)
    {
        BOOST_CHECK(mutexes[i].is_locked);
    }
}

void try_lock_two_uncontended()
{
    dummy_mutex m1,m2;

    int const res=boost::try_lock(m1,m2);
    
    BOOST_CHECK(res==-1);
    BOOST_CHECK(m1.is_locked);
    BOOST_CHECK(m2.is_locked);
}

void try_lock_two_first_locked()
{
    dummy_mutex m1,m2;
    m1.lock();

    boost::unique_lock<dummy_mutex> l1(m1,boost::defer_lock),
        l2(m2,boost::defer_lock);

    int const res=boost::try_lock(l1,l2);
    
    BOOST_CHECK(res==0);
    BOOST_CHECK(m1.is_locked);
    BOOST_CHECK(!m2.is_locked);
    BOOST_CHECK(!l1.owns_lock());
    BOOST_CHECK(!l2.owns_lock());
}

void try_lock_two_second_locked()
{
    dummy_mutex m1,m2;
    m2.lock();

    boost::unique_lock<dummy_mutex> l1(m1,boost::defer_lock),
        l2(m2,boost::defer_lock);

    int const res=boost::try_lock(l1,l2);
    
    BOOST_CHECK(res==1);
    BOOST_CHECK(!m1.is_locked);
    BOOST_CHECK(m2.is_locked);
    BOOST_CHECK(!l1.owns_lock());
    BOOST_CHECK(!l2.owns_lock());
}

void try_lock_three()
{
    int const num_mutexes=3;
    
    for(int i=-1;i<num_mutexes;++i)
    {
        dummy_mutex mutexes[num_mutexes];

        if(i>=0)
        {
            mutexes[i].lock();
        }
        boost::unique_lock<dummy_mutex> l1(mutexes[0],boost::defer_lock),
            l2(mutexes[1],boost::defer_lock),
            l3(mutexes[2],boost::defer_lock);

        int const res=boost::try_lock(l1,l2,l3);
    
        BOOST_CHECK(res==i);
        for(int j=0;j<num_mutexes;++j)
        {
            if((i==j) || (i==-1))
            {
                BOOST_CHECK(mutexes[j].is_locked);
            }
            else
            {
                BOOST_CHECK(!mutexes[j].is_locked);
            }
        }
        if(i==-1)
        {
            BOOST_CHECK(l1.owns_lock());
            BOOST_CHECK(l2.owns_lock());
            BOOST_CHECK(l3.owns_lock());
        }
        else
        {
            BOOST_CHECK(!l1.owns_lock());
            BOOST_CHECK(!l2.owns_lock());
            BOOST_CHECK(!l3.owns_lock());
        }
    }
}

void try_lock_four()
{
    int const num_mutexes=4;
    
    for(int i=-1;i<num_mutexes;++i)
    {
        dummy_mutex mutexes[num_mutexes];

        if(i>=0)
        {
            mutexes[i].lock();
        }
        boost::unique_lock<dummy_mutex> l1(mutexes[0],boost::defer_lock),
            l2(mutexes[1],boost::defer_lock),
            l3(mutexes[2],boost::defer_lock),
            l4(mutexes[3],boost::defer_lock);

        int const res=boost::try_lock(l1,l2,l3,l4);
    
        BOOST_CHECK(res==i);
        for(int j=0;j<num_mutexes;++j)
        {
            if((i==j) || (i==-1))
            {
                BOOST_CHECK(mutexes[j].is_locked);
            }
            else
            {
                BOOST_CHECK(!mutexes[j].is_locked);
            }
        }
        if(i==-1)
        {
            BOOST_CHECK(l1.owns_lock());
            BOOST_CHECK(l2.owns_lock());
            BOOST_CHECK(l3.owns_lock());
            BOOST_CHECK(l4.owns_lock());
        }
        else
        {
            BOOST_CHECK(!l1.owns_lock());
            BOOST_CHECK(!l2.owns_lock());
            BOOST_CHECK(!l3.owns_lock());
            BOOST_CHECK(!l4.owns_lock());
        }
    }
}

void try_lock_five()
{
    int const num_mutexes=5;
    
    for(int i=-1;i<num_mutexes;++i)
    {
        dummy_mutex mutexes[num_mutexes];

        if(i>=0)
        {
            mutexes[i].lock();
        }
        boost::unique_lock<dummy_mutex> l1(mutexes[0],boost::defer_lock),
            l2(mutexes[1],boost::defer_lock),
            l3(mutexes[2],boost::defer_lock),
            l4(mutexes[3],boost::defer_lock),
            l5(mutexes[4],boost::defer_lock);

        int const res=boost::try_lock(l1,l2,l3,l4,l5);
    
        BOOST_CHECK(res==i);
        for(int j=0;j<num_mutexes;++j)
        {
            if((i==j) || (i==-1))
            {
                BOOST_CHECK(mutexes[j].is_locked);
            }
            else
            {
                BOOST_CHECK(!mutexes[j].is_locked);
            }
        }
        if(i==-1)
        {
            BOOST_CHECK(l1.owns_lock());
            BOOST_CHECK(l2.owns_lock());
            BOOST_CHECK(l3.owns_lock());
            BOOST_CHECK(l4.owns_lock());
            BOOST_CHECK(l5.owns_lock());
        }
        else
        {
            BOOST_CHECK(!l1.owns_lock());
            BOOST_CHECK(!l2.owns_lock());
            BOOST_CHECK(!l3.owns_lock());
            BOOST_CHECK(!l4.owns_lock());
            BOOST_CHECK(!l5.owns_lock());
        }
    }
}

void test_lock_two_uncontended()
{
    stm::fiber( lock_two_uncontended).join();
    lock_two_uncontended();
}

void test_lock_five_uncontended()
{
    stm::fiber( lock_five_uncontended).join();
    lock_five_uncontended();
}

void test_lock_five_in_range()
{
    stm::fiber( lock_five_in_range).join();
    lock_five_in_range();
}

void test_lock_ten_in_range()
{
    stm::fiber( lock_ten_in_range).join();
    lock_ten_in_range();
}

void test_try_lock_two_uncontended()
{
    stm::fiber( try_lock_two_uncontended).join();
    try_lock_two_uncontended();
}

void test_try_lock_two_first_locked()
{
    stm::fiber( try_lock_two_first_locked).join();
    try_lock_two_first_locked();
}

void test_try_lock_two_second_locked()
{
    stm::fiber( try_lock_two_second_locked).join();
    try_lock_two_second_locked();
}

void test_try_lock_three()
{
    stm::fiber( try_lock_three).join();
    try_lock_three();
}

void test_try_lock_four()
{
    stm::fiber( try_lock_four).join();
    try_lock_four();
}

void test_try_lock_five()
{
    stm::fiber( try_lock_five).join();
    try_lock_five();
}

boost::unit_test_framework::test_suite* init_unit_test_suite(int, char*[])
{
    boost::unit_test::test_suite * test =
        BOOST_TEST_SUITE("Boost.Stratified: generic locks test suite");

    test->add(BOOST_TEST_CASE(&test_lock_two_uncontended));
    test->add(BOOST_TEST_CASE(&test_lock_five_uncontended));
    test->add(BOOST_TEST_CASE(&test_lock_five_in_range));
    test->add(BOOST_TEST_CASE(&test_lock_ten_in_range));
    test->add(BOOST_TEST_CASE(&test_try_lock_two_uncontended));
    test->add(BOOST_TEST_CASE(&test_try_lock_two_first_locked));
    test->add(BOOST_TEST_CASE(&test_try_lock_two_second_locked));
    test->add(BOOST_TEST_CASE(&test_try_lock_three));
    test->add(BOOST_TEST_CASE(&test_try_lock_four));
    test->add(BOOST_TEST_CASE(&test_try_lock_five));

    return test;
}
