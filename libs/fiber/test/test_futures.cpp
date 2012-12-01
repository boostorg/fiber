//  (C) Copyright 2008-10 Anthony Williams 
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <utility>
#include <memory>
#include <string>

#include <boost/chrono/system_clocks.hpp>
#include <boost/move/move.hpp>
#include <boost/fiber/all.hpp>
#include <boost/test/unit_test.hpp>

namespace stm = boost::fibers;

stm::mutex callback_mutex;
unsigned callback_called=0;
int global_ref_target=0;

int& return_ref()
{
    return global_ref_target;
}

void do_nothing_callback(stm::promise<int>& /*pi*/)
{
    boost::lock_guard<stm::mutex> lk(callback_mutex);
    ++callback_called;
}

void f_wait_callback(stm::promise<int>& pi)
{
    boost::lock_guard<stm::mutex> lk(callback_mutex);
    ++callback_called;
    try
    {
        pi.set_value(42);
    }
    catch(...)
    {
    }
}

void wait_callback_for_task(stm::packaged_task<int>& pt)
{
    boost::lock_guard<stm::mutex> lk(callback_mutex);
    ++callback_called;
    try
    {
        pt();
    }
    catch(...)
    {
    }
}

int make_int_slowly()
{
    boost::this_fiber::sleep(boost::chrono::seconds(1));
    return 42;
}

int echo( int i)
{
    boost::this_fiber::yield();
    return i;
}


void future_wait()
{
    stm::packaged_task<int> pt(boost::bind( echo, 42));
    stm::unique_future<int> f(pt.get_future());
    
    stm::fiber s( boost::move(pt) );
    
    f.wait();
    int i = f.get();
    
    BOOST_CHECK(f.is_ready());
    BOOST_CHECK_EQUAL( 42, i);
}


void do_nothing()
{}

struct X
{
private:
    BOOST_MOVABLE_BUT_NOT_COPYABLE( X)
    
public:
    int i;
    
    X():
        i(42)
    {}
#ifndef BOOST_NO_RVALUE_REFERENCES
    X(X&& other):
        i(other.i)
    {
        other.i=0;
    }
#else
    X( BOOST_RV_REF( X) other):
        i(other.i)
    {
        other.i=0;
    }
#endif
    ~X()
    {}
};

int make_int()
{ return 42; }

int throw_runtime_error()
{ throw std::runtime_error("42"); }

void set_promise_thread(stm::promise<int>* p)
{ p->set_value(42); }

struct my_exception
{};

void set_promise_exception_thread(stm::promise<int>* p)
{ p->set_exception(boost::copy_exception(my_exception())); }


void store_value_from_thread()
{
    stm::promise<int> pi2;
    stm::unique_future<int> fi2(pi2.get_future());
    stm::fiber s(
        boost::bind( set_promise_thread,&pi2) );
    int j=fi2.get();
    BOOST_CHECK(j==42);
    BOOST_CHECK(fi2.is_ready());
    BOOST_CHECK(fi2.has_value());
    BOOST_CHECK(!fi2.has_exception());
    BOOST_CHECK(fi2.get_state()==stm::future_state::ready);
}


void store_exception()
{
    stm::promise<int> pi3;
    stm::unique_future<int> fi3=pi3.get_future();
    stm::fiber s(
        boost::bind( set_promise_exception_thread,&pi3) );
    try
    {
        fi3.get();
        BOOST_CHECK(false);
    }
    catch(my_exception)
    {
        BOOST_CHECK(true);
    }
    
    BOOST_CHECK(fi3.is_ready());
    BOOST_CHECK(!fi3.has_value());
    BOOST_CHECK(fi3.has_exception());
    BOOST_CHECK(fi3.get_state()==stm::future_state::ready);
}

void initial_state()
{
    stm::unique_future<int> fi;
    BOOST_CHECK(!fi.is_ready());
    BOOST_CHECK(!fi.has_value());
    BOOST_CHECK(!fi.has_exception());
    BOOST_CHECK(fi.get_state()==stm::future_state::uninitialized);
    int i = 0;
    try
    {
        i=fi.get();
        BOOST_CHECK(0!=i);
        BOOST_CHECK(false);
    }
    catch(stm::future_uninitialized)
    {
        BOOST_CHECK(true);
    }
}

void waiting_future()
{
    stm::promise<int> pi;
    stm::unique_future<int> fi;
    fi=pi.get_future();

    int i=0;
    BOOST_CHECK(!fi.is_ready());
    BOOST_CHECK(!fi.has_value());
    BOOST_CHECK(!fi.has_exception());
    BOOST_CHECK(fi.get_state()==stm::future_state::waiting);
    BOOST_CHECK(i==0);
}

void cannot_get_future_twice()
{
    stm::promise<int> pi;
    pi.get_future();

    try
    {
        pi.get_future();
        BOOST_CHECK(false);
    }
    catch(stm::future_already_retrieved&)
    {
        BOOST_CHECK(true);
    }
}

void set_value_updates_future_state()
{
    stm::promise<int> pi;
    stm::unique_future<int> fi;
    fi=pi.get_future();

    pi.set_value(42);
    
    BOOST_CHECK(fi.is_ready());
    BOOST_CHECK(fi.has_value());
    BOOST_CHECK(!fi.has_exception());
    BOOST_CHECK(fi.get_state()==stm::future_state::ready);
}

void set_value_can_be_retrieved()
{
    stm::promise<int> pi;
    stm::unique_future<int> fi;
    fi=pi.get_future();

    pi.set_value(42);
    
    int i=0;
    BOOST_CHECK(i=fi.get());
    BOOST_CHECK(i==42);
    BOOST_CHECK(fi.is_ready());
    BOOST_CHECK(fi.has_value());
    BOOST_CHECK(!fi.has_exception());
    BOOST_CHECK(fi.get_state()==stm::future_state::ready);
}

void set_value_can_be_moved()
{
//     stm::promise<int> pi;
//     stm::unique_future<int> fi;
//     fi=pi.get_future();

//     pi.set_value(42);
    
//     int i=0;
//     BOOST_CHECK(i=fi.get());
//     BOOST_CHECK(i==42);
//     BOOST_CHECK(fi.is_ready());
//     BOOST_CHECK(fi.has_value());
//     BOOST_CHECK(!fi.has_exception());
//     BOOST_CHECK(fi.get_state()==stm::future_state::ready);
}

void future_from_packaged_task_is_waiting()
{
    stm::packaged_task<int> pt(make_int);
    stm::unique_future<int> fi=pt.get_future();
    int i=0;
    BOOST_CHECK(!fi.is_ready());
    BOOST_CHECK(!fi.has_value());
    BOOST_CHECK(!fi.has_exception());
    BOOST_CHECK(fi.get_state()==stm::future_state::waiting);
    BOOST_CHECK(i==0);
}

void invoking_a_packaged_task_populates_future()
{
    stm::packaged_task<int> pt(make_int);
    stm::unique_future<int> fi=pt.get_future();

    pt();

    int i=0;
    BOOST_CHECK(fi.is_ready());
    BOOST_CHECK(fi.has_value());
    BOOST_CHECK(!fi.has_exception());
    BOOST_CHECK(fi.get_state()==stm::future_state::ready);
    BOOST_CHECK(i=fi.get());
    BOOST_CHECK(i==42);
}

void invoking_a_packaged_task_twice_throws()
{
    stm::packaged_task<int> pt(make_int);

    pt();
    try
    {
        pt();
        BOOST_CHECK(false);
    }
    catch(stm::task_already_started)
    {
        BOOST_CHECK(true);
    }
}


void cannot_get_future_twice_from_task()
{
    stm::packaged_task<int> pt(make_int);
    pt.get_future();
    try
    {
        pt.get_future();
        BOOST_CHECK(false);
    }
    catch(stm::future_already_retrieved)
    {
        BOOST_CHECK(true);
    }
}

void task_stores_exception_if_function_throws()
{
    stm::packaged_task<int> pt(throw_runtime_error);
    stm::unique_future<int> fi=pt.get_future();

    pt();

    BOOST_CHECK(fi.is_ready());
    BOOST_CHECK(!fi.has_value());
    BOOST_CHECK(fi.has_exception());
    BOOST_CHECK(fi.get_state()==stm::future_state::ready);
    try
    {
        fi.get();
        BOOST_CHECK(false);
    }
    catch(std::exception&)
    {
        BOOST_CHECK(true);
    }
    catch(...)
    {
        BOOST_CHECK(!"Unknown exception thrown");
    }
    
}

void void_promise()
{
    stm::promise<void> p;
    stm::unique_future<void> f=p.get_future();
    p.set_value();
    BOOST_CHECK(f.is_ready());
    BOOST_CHECK(f.has_value());
    BOOST_CHECK(!f.has_exception());
    BOOST_CHECK(f.get_state()==stm::future_state::ready);
    f.get();
}

void reference_promise()
{
    stm::promise<int&> p;
    stm::unique_future<int&> f=p.get_future();
    int i=42;
    p.set_value(i);
    BOOST_CHECK(f.is_ready());
    BOOST_CHECK(f.has_value());
    BOOST_CHECK(!f.has_exception());
    BOOST_CHECK(f.get_state()==stm::future_state::ready);
    BOOST_CHECK(&f.get()==&i);
}

void task_returning_void()
{
    stm::packaged_task<void> pt(do_nothing);
    stm::unique_future<void> fi=pt.get_future();

    pt();

    BOOST_CHECK(fi.is_ready());
    BOOST_CHECK(fi.has_value());
    BOOST_CHECK(!fi.has_exception());
    BOOST_CHECK(fi.get_state()==stm::future_state::ready);
}

void task_returning_reference()
{
    stm::packaged_task<int&> pt(return_ref);
    stm::unique_future<int&> fi=pt.get_future();

    pt();

    BOOST_CHECK(fi.is_ready());
    BOOST_CHECK(fi.has_value());
    BOOST_CHECK(!fi.has_exception());
    BOOST_CHECK(fi.get_state()==stm::future_state::ready);
    int& i=fi.get();
    BOOST_CHECK(&i==&global_ref_target);
}

void shared_future()
{
    stm::packaged_task<int> pt(make_int);
    stm::unique_future<int> fi=pt.get_future();

    stm::shared_future<int> sf(boost::move(fi));
    BOOST_CHECK(fi.get_state()==stm::future_state::uninitialized);

    pt();

    int i=0;
    BOOST_CHECK(sf.is_ready());
    BOOST_CHECK(sf.has_value());
    BOOST_CHECK(!sf.has_exception());
    BOOST_CHECK(sf.get_state()==stm::future_state::ready);
    BOOST_CHECK(i=sf.get());
    BOOST_CHECK(i==42);
}

void copies_of_shared_future_become_ready_together()
{
    stm::packaged_task<int> pt(make_int);
    stm::unique_future<int> fi=pt.get_future();

    stm::shared_future<int> sf(boost::move(fi));
    stm::shared_future<int> sf2(sf);
    stm::shared_future<int> sf3;
    sf3=sf;
    BOOST_CHECK(sf.get_state()==stm::future_state::waiting);
    BOOST_CHECK(sf2.get_state()==stm::future_state::waiting);
    BOOST_CHECK(sf3.get_state()==stm::future_state::waiting);

    pt();

    int i=0;
    BOOST_CHECK(sf.is_ready());
    BOOST_CHECK(sf.has_value());
    BOOST_CHECK(!sf.has_exception());
    BOOST_CHECK(sf.get_state()==stm::future_state::ready);
    BOOST_CHECK(i=sf.get());
    BOOST_CHECK(i==42);
    i=0;
    BOOST_CHECK(sf2.is_ready());
    BOOST_CHECK(sf2.has_value());
    BOOST_CHECK(!sf2.has_exception());
    BOOST_CHECK(sf2.get_state()==stm::future_state::ready);
    BOOST_CHECK(i=sf2.get());
    BOOST_CHECK(i==42);
    i=0;
    BOOST_CHECK(sf3.is_ready());
    BOOST_CHECK(sf3.has_value());
    BOOST_CHECK(!sf3.has_exception());
    BOOST_CHECK(sf3.get_state()==stm::future_state::ready);
    BOOST_CHECK(i=sf3.get());
    BOOST_CHECK(i==42);
}

void shared_future_can_be_move_assigned_from_unique_future()
{
    stm::packaged_task<int> pt(make_int);
    stm::unique_future<int> fi=pt.get_future();

    stm::shared_future<int> sf;
    sf=boost::move(fi);
    BOOST_CHECK(fi.get_state()==stm::future_state::uninitialized);

    BOOST_CHECK(!sf.is_ready());
    BOOST_CHECK(!sf.has_value());
    BOOST_CHECK(!sf.has_exception());
    BOOST_CHECK(sf.get_state()==stm::future_state::waiting);
}

void shared_future_void()
{
    stm::packaged_task<void> pt(do_nothing);
    stm::unique_future<void> fi=pt.get_future();

    stm::shared_future<void> sf(boost::move(fi));
    BOOST_CHECK(fi.get_state()==stm::future_state::uninitialized);

    pt();

    BOOST_CHECK(sf.is_ready());
    BOOST_CHECK(sf.has_value());
    BOOST_CHECK(!sf.has_exception());
    BOOST_CHECK(sf.get_state()==stm::future_state::ready);
    sf.get();
}

void shared_future_ref()
{
    stm::promise<int&> p;
    stm::shared_future<int&> f(p.get_future());
    int i=42;
    p.set_value(i);
    BOOST_CHECK(f.is_ready());
    BOOST_CHECK(f.has_value());
    BOOST_CHECK(!f.has_exception());
    BOOST_CHECK(f.get_state()==stm::future_state::ready);
    BOOST_CHECK(&f.get()==&i);
}

void can_get_a_second_future_from_a_moved_promise()
{
    stm::promise<int> pi;
    stm::unique_future<int> fi=pi.get_future();
    
    stm::promise<int> pi2(boost::move(pi));
    stm::unique_future<int> fi2=pi.get_future();

    pi2.set_value(3);
    BOOST_CHECK(fi.is_ready());
    BOOST_CHECK(!fi2.is_ready());
    BOOST_CHECK(fi.get()==3);
    pi.set_value(42);
    BOOST_CHECK(fi2.is_ready());
    BOOST_CHECK(fi2.get()==42);
}

void can_get_a_second_future_from_a_moved_void_promise()
{
    stm::promise<void> pi;
    stm::unique_future<void> fi=pi.get_future();
    
    stm::promise<void> pi2(boost::move(pi));
    stm::unique_future<void> fi2=pi.get_future();

    pi2.set_value();
    BOOST_CHECK(fi.is_ready());
    BOOST_CHECK(!fi2.is_ready());
    pi.set_value();
    BOOST_CHECK(fi2.is_ready());
}
#if 0
void unique_future_for_move_only_udt()
{
    stm::promise<X> pt;
    stm::unique_future<X> fi=pt.get_future();

    X tmp;
    pt.set_value(boost::move(tmp));
    X res(boost::move(fi.get()));
    BOOST_CHECK(res.i==42);
}
#endif
void unique_future_for_string()
{
    stm::promise<std::string> pt;
    stm::unique_future<std::string> fi=pt.get_future();

    pt.set_value(std::string("hello"));
    std::string res(fi.get());
    BOOST_CHECK(res=="hello");

    stm::promise<std::string> pt2;
    fi=pt2.get_future();

    std::string const s="goodbye";
    
    pt2.set_value(s);
    res=fi.get();
    BOOST_CHECK(res=="goodbye");

    stm::promise<std::string> pt3;
    fi=pt3.get_future();

    std::string s2="foo";
    
    pt3.set_value(s2);
    res=fi.get();
    BOOST_CHECK(res=="foo");
}

void wait_callback()
{
    callback_called=0;
    stm::promise<int> pi;
    stm::unique_future<int> fi=pi.get_future();
    pi.set_wait_callback(f_wait_callback);
    fi.wait();
    BOOST_CHECK(callback_called);
    BOOST_CHECK(fi.get()==42);
    fi.wait();
    fi.wait();
    BOOST_CHECK(callback_called==1);
}

void wait_callback_with_timed_wait()
{
    callback_called=0;
    stm::promise<int> pi;
    stm::unique_future<int> fi=pi.get_future();
    pi.set_wait_callback(do_nothing_callback);
    bool success=fi.timed_wait(boost::chrono::milliseconds(10));
    BOOST_CHECK(callback_called);
    BOOST_CHECK(!success);
    success=fi.timed_wait(boost::chrono::milliseconds(10));
    BOOST_CHECK(!success);
    success=fi.timed_wait(boost::chrono::milliseconds(10));
    BOOST_CHECK(!success);
    BOOST_CHECK(callback_called==3);
    pi.set_value(42);
    success=fi.timed_wait(boost::chrono::milliseconds(10));
    BOOST_CHECK(success);
    BOOST_CHECK(callback_called==3);
    BOOST_CHECK(fi.get()==42);
    BOOST_CHECK(callback_called==3);
}

void wait_callback_for_packaged_task()
{
    callback_called=0;
    stm::packaged_task<int> pt(make_int);
    stm::unique_future<int> fi=pt.get_future();
    pt.set_wait_callback(wait_callback_for_task);
    fi.wait();
    BOOST_CHECK(callback_called);
    BOOST_CHECK(fi.get()==42);
    fi.wait();
    fi.wait();
    BOOST_CHECK(callback_called==1);
}

void packaged_task_can_be_moved()
{
    stm::packaged_task<int> pt(make_int);

    stm::unique_future<int> fi=pt.get_future();

    BOOST_CHECK(!fi.is_ready());
    
    stm::packaged_task<int> pt2(boost::move(pt));

    BOOST_CHECK(!fi.is_ready());
    try
    {
        pt();
        BOOST_CHECK(!"Can invoke moved task!");
    }
    catch(stm::task_moved&)
    {
    }

    BOOST_CHECK(!fi.is_ready());

    pt2();
    
    BOOST_CHECK(fi.is_ready());
}

void destroying_a_promise_stores_broken_promise()
{
    stm::unique_future<int> f;
    
    {
        stm::promise<int> p;
        f=p.get_future();
    }
    BOOST_CHECK(f.is_ready());
    BOOST_CHECK(f.has_exception());
    try
    {
        f.get();
    }
    catch(stm::broken_promise&)
    {
    }
}

void destroying_a_packaged_task_stores_broken_promise()
{
    stm::unique_future<int> f;
    
    {
        stm::packaged_task<int> p(make_int);
        f=p.get_future();
    }
    BOOST_CHECK(f.is_ready());
    BOOST_CHECK(f.has_exception());
    try
    {
        f.get();
    }
    catch(stm::broken_promise&)
    {
    }
}

void wait_for_either_of_two_futures_1()
{
    stm::packaged_task<int> pt(make_int_slowly);
    stm::unique_future<int> f1(pt.get_future());
    stm::packaged_task<int> pt2(make_int_slowly);
    stm::unique_future<int> f2(pt2.get_future());
    
    stm::fiber s( boost::move(pt) );
    
    unsigned const future=stm::waitfor_any(f1,f2);
    
    BOOST_CHECK(future==0);
    BOOST_CHECK(f1.is_ready());
    BOOST_CHECK(!f2.is_ready());
    BOOST_CHECK(f1.get()==42);
}

void wait_for_either_of_two_futures_2()
{
    stm::packaged_task<int> pt(make_int_slowly);
    stm::unique_future<int> f1(pt.get_future());
    stm::packaged_task<int> pt2(make_int_slowly);
    stm::unique_future<int> f2(pt2.get_future());
    
    stm::fiber s( boost::move(pt2) );
    
    unsigned const future=stm::waitfor_any(f1,f2);
    
    BOOST_CHECK(future==1);
    BOOST_CHECK(!f1.is_ready());
    BOOST_CHECK(f2.is_ready());
    BOOST_CHECK(f2.get()==42);
}

void wait_for_either_of_three_futures_1()
{
    stm::packaged_task<int> pt(make_int_slowly);
    stm::unique_future<int> f1(pt.get_future());
    stm::packaged_task<int> pt2(make_int_slowly);
    stm::unique_future<int> f2(pt2.get_future());
    stm::packaged_task<int> pt3(make_int_slowly);
    stm::unique_future<int> f3(pt3.get_future());
    
    stm::fiber s( boost::move(pt) );
    
    unsigned const future=stm::waitfor_any(f1,f2,f3);
    
    BOOST_CHECK(future==0);
    BOOST_CHECK(f1.is_ready());
    BOOST_CHECK(!f2.is_ready());
    BOOST_CHECK(!f3.is_ready());
    BOOST_CHECK(f1.get()==42);
}

void wait_for_either_of_three_futures_2()
{
    stm::packaged_task<int> pt(make_int_slowly);
    stm::unique_future<int> f1(pt.get_future());
    stm::packaged_task<int> pt2(make_int_slowly);
    stm::unique_future<int> f2(pt2.get_future());
    stm::packaged_task<int> pt3(make_int_slowly);
    stm::unique_future<int> f3(pt3.get_future());
    
    stm::fiber s( boost::move(pt2) );
    
    unsigned const future=stm::waitfor_any(f1,f2,f3);
    
    BOOST_CHECK(future==1);
    BOOST_CHECK(!f1.is_ready());
    BOOST_CHECK(f2.is_ready());
    BOOST_CHECK(!f3.is_ready());
    BOOST_CHECK(f2.get()==42);
}

void wait_for_either_of_three_futures_3()
{
    stm::packaged_task<int> pt(make_int_slowly);
    stm::unique_future<int> f1(pt.get_future());
    stm::packaged_task<int> pt2(make_int_slowly);
    stm::unique_future<int> f2(pt2.get_future());
    stm::packaged_task<int> pt3(make_int_slowly);
    stm::unique_future<int> f3(pt3.get_future());
    
    stm::fiber s( boost::move(pt3) );
    
    unsigned const future=stm::waitfor_any(f1,f2,f3);
    
    BOOST_CHECK(future==2);
    BOOST_CHECK(!f1.is_ready());
    BOOST_CHECK(!f2.is_ready());
    BOOST_CHECK(f3.is_ready());
    BOOST_CHECK(f3.get()==42);
}

void wait_for_either_of_four_futures_1()
{
    stm::packaged_task<int> pt(make_int_slowly);
    stm::unique_future<int> f1(pt.get_future());
    stm::packaged_task<int> pt2(make_int_slowly);
    stm::unique_future<int> f2(pt2.get_future());
    stm::packaged_task<int> pt3(make_int_slowly);
    stm::unique_future<int> f3(pt3.get_future());
    stm::packaged_task<int> pt4(make_int_slowly);
    stm::unique_future<int> f4(pt4.get_future());
    
    stm::fiber(boost::move(pt));
    
    unsigned const future=stm::waitfor_any(f1,f2,f3,f4);
    
    BOOST_CHECK(future==0);
    BOOST_CHECK(f1.is_ready());
    BOOST_CHECK(!f2.is_ready());
    BOOST_CHECK(!f3.is_ready());
    BOOST_CHECK(!f4.is_ready());
    BOOST_CHECK(f1.get()==42);
}

void wait_for_either_of_four_futures_2()
{
    stm::packaged_task<int> pt(make_int_slowly);
    stm::unique_future<int> f1(pt.get_future());
    stm::packaged_task<int> pt2(make_int_slowly);
    stm::unique_future<int> f2(pt2.get_future());
    stm::packaged_task<int> pt3(make_int_slowly);
    stm::unique_future<int> f3(pt3.get_future());
    stm::packaged_task<int> pt4(make_int_slowly);
    stm::unique_future<int> f4(pt4.get_future());
    
    stm::fiber(boost::move(pt2));
    
    unsigned const future=stm::waitfor_any(f1,f2,f3,f4);
    
    BOOST_CHECK(future==1);
    BOOST_CHECK(!f1.is_ready());
    BOOST_CHECK(f2.is_ready());
    BOOST_CHECK(!f3.is_ready());
    BOOST_CHECK(!f4.is_ready());
    BOOST_CHECK(f2.get()==42);
}

void wait_for_either_of_four_futures_3()
{
    stm::packaged_task<int> pt(make_int_slowly);
    stm::unique_future<int> f1(pt.get_future());
    stm::packaged_task<int> pt2(make_int_slowly);
    stm::unique_future<int> f2(pt2.get_future());
    stm::packaged_task<int> pt3(make_int_slowly);
    stm::unique_future<int> f3(pt3.get_future());
    stm::packaged_task<int> pt4(make_int_slowly);
    stm::unique_future<int> f4(pt4.get_future());
    
    stm::fiber(boost::move(pt3));
    
    unsigned const future=stm::waitfor_any(f1,f2,f3,f4);
    
    BOOST_CHECK(future==2);
    BOOST_CHECK(!f1.is_ready());
    BOOST_CHECK(!f2.is_ready());
    BOOST_CHECK(f3.is_ready());
    BOOST_CHECK(!f4.is_ready());
    BOOST_CHECK(f3.get()==42);
}

void wait_for_either_of_four_futures_4()
{
    stm::packaged_task<int> pt(make_int_slowly);
    stm::unique_future<int> f1(pt.get_future());
    stm::packaged_task<int> pt2(make_int_slowly);
    stm::unique_future<int> f2(pt2.get_future());
    stm::packaged_task<int> pt3(make_int_slowly);
    stm::unique_future<int> f3(pt3.get_future());
    stm::packaged_task<int> pt4(make_int_slowly);
    stm::unique_future<int> f4(pt4.get_future());
    
    stm::fiber(boost::move(pt4));
    
    unsigned const future=stm::waitfor_any(f1,f2,f3,f4);
    
    BOOST_CHECK(future==3);
    BOOST_CHECK(!f1.is_ready());
    BOOST_CHECK(!f2.is_ready());
    BOOST_CHECK(!f3.is_ready());
    BOOST_CHECK(f4.is_ready());
    BOOST_CHECK(f4.get()==42);
}

void wait_for_either_of_five_futures_1()
{
    stm::packaged_task<int> pt(make_int_slowly);
    stm::unique_future<int> f1(pt.get_future());
    stm::packaged_task<int> pt2(make_int_slowly);
    stm::unique_future<int> f2(pt2.get_future());
    stm::packaged_task<int> pt3(make_int_slowly);
    stm::unique_future<int> f3(pt3.get_future());
    stm::packaged_task<int> pt4(make_int_slowly);
    stm::unique_future<int> f4(pt4.get_future());
    stm::packaged_task<int> pt5(make_int_slowly);
    stm::unique_future<int> f5(pt5.get_future());
    
    stm::fiber(boost::move(pt));
    
    unsigned const future=stm::waitfor_any(f1,f2,f3,f4,f5);
    
    BOOST_CHECK(future==0);
    BOOST_CHECK(f1.is_ready());
    BOOST_CHECK(!f2.is_ready());
    BOOST_CHECK(!f3.is_ready());
    BOOST_CHECK(!f4.is_ready());
    BOOST_CHECK(!f5.is_ready());
    BOOST_CHECK(f1.get()==42);
}

void wait_for_either_of_five_futures_2()
{
    stm::packaged_task<int> pt(make_int_slowly);
    stm::unique_future<int> f1(pt.get_future());
    stm::packaged_task<int> pt2(make_int_slowly);
    stm::unique_future<int> f2(pt2.get_future());
    stm::packaged_task<int> pt3(make_int_slowly);
    stm::unique_future<int> f3(pt3.get_future());
    stm::packaged_task<int> pt4(make_int_slowly);
    stm::unique_future<int> f4(pt4.get_future());
    stm::packaged_task<int> pt5(make_int_slowly);
    stm::unique_future<int> f5(pt5.get_future());
    
    stm::fiber(boost::move(pt2));
    
    unsigned const future=stm::waitfor_any(f1,f2,f3,f4,f5);
    
    BOOST_CHECK(future==1);
    BOOST_CHECK(!f1.is_ready());
    BOOST_CHECK(f2.is_ready());
    BOOST_CHECK(!f3.is_ready());
    BOOST_CHECK(!f4.is_ready());
    BOOST_CHECK(!f5.is_ready());
    BOOST_CHECK(f2.get()==42);
}
void wait_for_either_of_five_futures_3()
{
    stm::packaged_task<int> pt(make_int_slowly);
    stm::unique_future<int> f1(pt.get_future());
    stm::packaged_task<int> pt2(make_int_slowly);
    stm::unique_future<int> f2(pt2.get_future());
    stm::packaged_task<int> pt3(make_int_slowly);
    stm::unique_future<int> f3(pt3.get_future());
    stm::packaged_task<int> pt4(make_int_slowly);
    stm::unique_future<int> f4(pt4.get_future());
    stm::packaged_task<int> pt5(make_int_slowly);
    stm::unique_future<int> f5(pt5.get_future());
    
    stm::fiber(boost::move(pt3));
    
    unsigned const future=stm::waitfor_any(f1,f2,f3,f4,f5);
    
    BOOST_CHECK(future==2);
    BOOST_CHECK(!f1.is_ready());
    BOOST_CHECK(!f2.is_ready());
    BOOST_CHECK(f3.is_ready());
    BOOST_CHECK(!f4.is_ready());
    BOOST_CHECK(!f5.is_ready());
    BOOST_CHECK(f3.get()==42);
}
void wait_for_either_of_five_futures_4()
{
    stm::packaged_task<int> pt(make_int_slowly);
    stm::unique_future<int> f1(pt.get_future());
    stm::packaged_task<int> pt2(make_int_slowly);
    stm::unique_future<int> f2(pt2.get_future());
    stm::packaged_task<int> pt3(make_int_slowly);
    stm::unique_future<int> f3(pt3.get_future());
    stm::packaged_task<int> pt4(make_int_slowly);
    stm::unique_future<int> f4(pt4.get_future());
    stm::packaged_task<int> pt5(make_int_slowly);
    stm::unique_future<int> f5(pt5.get_future());
    
    stm::fiber(boost::move(pt4));
    
    unsigned const future=stm::waitfor_any(f1,f2,f3,f4,f5);
    
    BOOST_CHECK(future==3);
    BOOST_CHECK(!f1.is_ready());
    BOOST_CHECK(!f2.is_ready());
    BOOST_CHECK(!f3.is_ready());
    BOOST_CHECK(f4.is_ready());
    BOOST_CHECK(!f5.is_ready());
    BOOST_CHECK(f4.get()==42);
}
void wait_for_either_of_five_futures_5()
{
    stm::packaged_task<int> pt(make_int_slowly);
    stm::unique_future<int> f1(pt.get_future());
    stm::packaged_task<int> pt2(make_int_slowly);
    stm::unique_future<int> f2(pt2.get_future());
    stm::packaged_task<int> pt3(make_int_slowly);
    stm::unique_future<int> f3(pt3.get_future());
    stm::packaged_task<int> pt4(make_int_slowly);
    stm::unique_future<int> f4(pt4.get_future());
    stm::packaged_task<int> pt5(make_int_slowly);
    stm::unique_future<int> f5(pt5.get_future());
    
    stm::fiber(boost::move(pt5));
    
    unsigned const future=stm::waitfor_any(f1,f2,f3,f4,f5);
    
    BOOST_CHECK(future==4);
    BOOST_CHECK(!f1.is_ready());
    BOOST_CHECK(!f2.is_ready());
    BOOST_CHECK(!f3.is_ready());
    BOOST_CHECK(!f4.is_ready());
    BOOST_CHECK(f5.is_ready());
    BOOST_CHECK(f5.get()==42);
}

void wait_for_either_invokes_callbacks()
{
    callback_called=0;
    stm::packaged_task<int> pt(make_int_slowly);
    stm::unique_future<int> fi=pt.get_future();
    stm::packaged_task<int> pt2(make_int_slowly);
    stm::unique_future<int> fi2=pt2.get_future();
    pt.set_wait_callback(wait_callback_for_task);

    stm::fiber f(boost::move(pt));
    stm::waitfor_any(fi,fi2);
    
    BOOST_CHECK(fi.get()==42);
    BOOST_CHECK(callback_called==1);
}
#if 0
void wait_for_any_from_range()
{
    unsigned const count=10;
    for(unsigned i=0;i<count;++i)
    {
        stm::packaged_task<int> tasks[count];
        stm::unique_future<int> futures[count];
        for(unsigned j=0;j<count;++j)
        {
            tasks[j]=stm::packaged_task<int>(make_int_slowly);
            futures[j]=tasks[j].get_future();
        }
        stm::fiber(boost::move(tasks[i]));
    
        BOOST_CHECK(stm::waitfor_any(futures,futures)==futures);
        
        stm::unique_future<int>* const future=stm::waitfor_any(futures,futures+count);
    
        BOOST_CHECK(future==(futures+i));
        for(unsigned j=0;j<count;++j)
        {
            if(j!=i)
            {
                BOOST_CHECK(!futures[j].is_ready());
            }
            else
            {
                BOOST_CHECK(futures[j].is_ready());
            }
        }
        BOOST_CHECK(futures[i].get()==42);
    }
}

void wait_for_all_from_range()
{
    unsigned const count=10;
    stm::unique_future<int> futures[count];
    for(unsigned j=0;j<count;++j)
    {
        stm::packaged_task<int> task(make_int_slowly);
        futures[j]=task.get_future();
        stm::fiber(boost::move(task));
    }
    
    stm::waitfor_all(futures,futures+count);
    
    for(unsigned j=0;j<count;++j)
    {
        BOOST_CHECK(futures[j].is_ready());
    }
}
#endif

void wait_for_all_two_futures()
{
    unsigned const count=2;
    stm::unique_future<int> futures[count];
    for(unsigned j=0;j<count;++j)
    {
        stm::packaged_task<int> task(make_int_slowly);
        futures[j]=task.get_future();
        stm::fiber(boost::move(task));
    }
    
    stm::waitfor_all(futures[0],futures[1]);
    
    for(unsigned j=0;j<count;++j)
    {
        BOOST_CHECK(futures[j].is_ready());
    }
}

void wait_for_all_three_futures()
{
    unsigned const count=3;
    stm::unique_future<int> futures[count];
    for(unsigned j=0;j<count;++j)
    {
        stm::packaged_task<int> task(make_int_slowly);
        futures[j]=task.get_future();
        stm::fiber(boost::move(task));
    }
    
    stm::waitfor_all(futures[0],futures[1],futures[2]);
    
    for(unsigned j=0;j<count;++j)
    {
        BOOST_CHECK(futures[j].is_ready());
    }
}

void wait_for_all_four_futures()
{
    unsigned const count=4;
    stm::unique_future<int> futures[count];
    for(unsigned j=0;j<count;++j)
    {
        stm::packaged_task<int> task(make_int_slowly);
        futures[j]=task.get_future();
        stm::fiber(boost::move(task));
    }
    
    stm::waitfor_all(futures[0],futures[1],futures[2],futures[3]);
    
    for(unsigned j=0;j<count;++j)
    {
        BOOST_CHECK(futures[j].is_ready());
    }
}

void wait_for_all_five_futures()
{
    unsigned const count=5;
    stm::unique_future<int> futures[count];
    for(unsigned j=0;j<count;++j)
    {
        stm::packaged_task<int> task(make_int_slowly);
        futures[j]=task.get_future();
        stm::fiber(boost::move(task));
    }
    
    stm::waitfor_all(futures[0],futures[1],futures[2],futures[3],futures[4]);
    
    for(unsigned j=0;j<count;++j)
    {
        BOOST_CHECK(futures[j].is_ready());
    }
}


void test_store_value_from_thread()
{
    stm::fiber( store_value_from_thread).join();
    store_value_from_thread();
}

void test_store_exception()
{
    stm::fiber( store_exception).join();
    store_exception();
}

void test_initial_state()
{
    stm::fiber( initial_state).join();
    initial_state();
}

void test_waiting_future()
{
    stm::fiber( waiting_future).join();
    waiting_future();
}

void test_cannot_get_future_twice()
{
    stm::fiber( cannot_get_future_twice).join();
    cannot_get_future_twice();
}

void test_set_value_updates_future_state()
{
    stm::fiber( set_value_updates_future_state).join();
    set_value_updates_future_state();
}

void test_set_value_can_be_retrieved()
{
    stm::fiber( set_value_can_be_retrieved).join();
    set_value_can_be_retrieved();
}

void test_set_value_can_be_moved()
{
}

void test_future_from_packaged_task_is_waiting()
{
    stm::fiber( future_from_packaged_task_is_waiting).join();
    future_from_packaged_task_is_waiting();
}

void test_invoking_a_packaged_task_populates_future()
{
    stm::fiber( invoking_a_packaged_task_populates_future).join();
    invoking_a_packaged_task_populates_future();
}

void test_invoking_a_packaged_task_twice_throws()
{
    stm::fiber( invoking_a_packaged_task_twice_throws).join();
    invoking_a_packaged_task_twice_throws();
}

void test_cannot_get_future_twice_from_task()
{
    stm::fiber( cannot_get_future_twice_from_task).join();
    cannot_get_future_twice_from_task();
}

void test_task_stores_exception_if_function_throws()
{
    stm::fiber( task_stores_exception_if_function_throws).join();
    task_stores_exception_if_function_throws();
}

void test_void_promise()
{
    stm::fiber( void_promise).join();
    void_promise();
}

void test_reference_promise()
{
    stm::fiber( reference_promise).join();
    reference_promise();
}

void test_task_returning_void()
{
    stm::fiber( task_returning_void).join();
    task_returning_void();
}

void test_task_returning_reference()
{
    stm::fiber( task_returning_reference).join();
    task_returning_reference();
}

void test_shared_future()
{
    stm::fiber( shared_future).join();
    shared_future();
}

void test_copies_of_shared_future_become_ready_together()
{
    stm::fiber( copies_of_shared_future_become_ready_together).join();
    copies_of_shared_future_become_ready_together();
}

void test_shared_future_can_be_move_assigned_from_unique_future()
{
    stm::fiber( shared_future_can_be_move_assigned_from_unique_future).join();
    shared_future_can_be_move_assigned_from_unique_future();
}

void test_shared_future_void()
{
    stm::fiber( shared_future_void).join();
    shared_future_void();
}

void test_shared_future_ref()
{
    stm::fiber( shared_future_ref).join();
    shared_future_ref();
}

void test_can_get_a_second_future_from_a_moved_promise()
{
    stm::fiber( can_get_a_second_future_from_a_moved_promise).join();
    can_get_a_second_future_from_a_moved_promise();
}

void test_can_get_a_second_future_from_a_moved_void_promise()
{
    stm::fiber( can_get_a_second_future_from_a_moved_void_promise).join();
    can_get_a_second_future_from_a_moved_void_promise();
}
#if 0
void test_unique_future_for_move_only_udt()
{
    stm::fiber( unique_future_for_move_only_udt).join();
    unique_future_for_move_only_udt();
}
#endif
void test_unique_future_for_string()
{
    stm::fiber( unique_future_for_string).join();
    unique_future_for_string();
}

void test_wait_callback()
{
    stm::fiber( wait_callback).join();
    wait_callback();
}

void test_wait_callback_with_timed_wait()
{
    stm::fiber( wait_callback_with_timed_wait).join();
    wait_callback_with_timed_wait();
}

void test_wait_callback_for_packaged_task()
{
    stm::fiber( wait_callback_for_packaged_task).join();
    wait_callback_for_packaged_task();
}

void test_packaged_task_can_be_moved()
{
    stm::fiber( packaged_task_can_be_moved).join();
    packaged_task_can_be_moved();
}

void test_destroying_a_promise_stores_broken_promise()
{
    stm::fiber( destroying_a_promise_stores_broken_promise).join();
    destroying_a_promise_stores_broken_promise();
}

void test_destroying_a_packaged_task_stores_broken_promise()
{
    stm::fiber( destroying_a_packaged_task_stores_broken_promise).join();
    destroying_a_packaged_task_stores_broken_promise();
}

void test_wait_for_either_of_two_futures_1()
{
    stm::fiber( wait_for_either_of_two_futures_1).join();
    wait_for_either_of_two_futures_1();
}

void test_wait_for_either_of_two_futures_2()
{
    stm::fiber( wait_for_either_of_two_futures_2).join();
    wait_for_either_of_two_futures_2();
}

void test_wait_for_either_of_three_futures_1()
{
    stm::fiber( wait_for_either_of_three_futures_1).join();
    wait_for_either_of_three_futures_1();
}

void test_wait_for_either_of_three_futures_2()
{
    stm::fiber( wait_for_either_of_three_futures_2).join();
    wait_for_either_of_three_futures_2();
}

void test_wait_for_either_of_three_futures_3()
{
    stm::fiber( wait_for_either_of_three_futures_3).join();
    wait_for_either_of_three_futures_3();
}

void test_wait_for_either_of_four_futures_1()
{
    stm::fiber( wait_for_either_of_four_futures_1).join();
    wait_for_either_of_four_futures_1();
}

void test_wait_for_either_of_four_futures_2()
{
    stm::fiber( wait_for_either_of_four_futures_2).join();
    wait_for_either_of_four_futures_2();
}

void test_wait_for_either_of_four_futures_3()
{
    stm::fiber( wait_for_either_of_four_futures_3).join();
    wait_for_either_of_four_futures_3();
}

void test_wait_for_either_of_four_futures_4()
{
    stm::fiber( wait_for_either_of_four_futures_4).join();
    wait_for_either_of_four_futures_4();
}

void test_wait_for_either_of_five_futures_1()
{
    stm::fiber( wait_for_either_of_five_futures_1).join();
    wait_for_either_of_five_futures_1();
}

void test_wait_for_either_of_five_futures_2()
{
    stm::fiber( wait_for_either_of_five_futures_2).join();
    wait_for_either_of_five_futures_2();
}

void test_wait_for_either_of_five_futures_3()
{
    stm::fiber( wait_for_either_of_five_futures_3).join();
    wait_for_either_of_five_futures_3();
}

void test_wait_for_either_of_five_futures_4()
{
    stm::fiber( wait_for_either_of_five_futures_4).join();
    wait_for_either_of_five_futures_4();
}

void test_wait_for_either_of_five_futures_5()
{
    stm::fiber( wait_for_either_of_five_futures_5).join();
    wait_for_either_of_five_futures_5();
}

void test_wait_for_either_invokes_callbacks()
{
    stm::fiber( wait_for_either_invokes_callbacks).join();
    wait_for_either_invokes_callbacks();
}
#if 0
void test_wait_for_any_from_range()
{
    stm::fiber( wait_for_any_from_range).join();
    wait_for_any_from_range();
}

void test_wait_for_all_from_range()
{
    stm::fiber( wait_for_all_from_range).join();
    wait_for_all_from_range();
}
#endif
void test_wait_for_all_two_futures()
{
    stm::fiber( wait_for_all_two_futures).join();
    wait_for_all_two_futures();
}

void test_wait_for_all_three_futures()
{
    stm::fiber( wait_for_all_three_futures).join();
    wait_for_all_three_futures();
}

void test_wait_for_all_four_futures()
{
    stm::fiber( wait_for_all_four_futures).join();
    wait_for_all_four_futures();
}

void test_wait_for_all_five_futures()
{
    stm::fiber( wait_for_all_five_futures).join();
    wait_for_all_five_futures();
}

void test_future_wait()
{
    stm::fiber( future_wait).join();
    future_wait();
}

boost::unit_test_framework::test_suite* init_unit_test_suite(int, char*[])
{
    boost::unit_test_framework::test_suite* test =
        BOOST_TEST_SUITE("Boost.Threads: futures test suite");

    test->add(BOOST_TEST_CASE(test_initial_state));
    test->add(BOOST_TEST_CASE(test_waiting_future));
    test->add(BOOST_TEST_CASE(test_cannot_get_future_twice));
    test->add(BOOST_TEST_CASE(test_set_value_updates_future_state));
    test->add(BOOST_TEST_CASE(test_set_value_can_be_retrieved));
    test->add(BOOST_TEST_CASE(test_set_value_can_be_moved));
    test->add(BOOST_TEST_CASE(test_store_value_from_thread));
    test->add(BOOST_TEST_CASE(test_store_exception));
    test->add(BOOST_TEST_CASE(test_future_from_packaged_task_is_waiting));
    test->add(BOOST_TEST_CASE(test_invoking_a_packaged_task_populates_future));
    test->add(BOOST_TEST_CASE(test_invoking_a_packaged_task_twice_throws));
    test->add(BOOST_TEST_CASE(test_cannot_get_future_twice_from_task));
    test->add(BOOST_TEST_CASE(test_task_stores_exception_if_function_throws));
    test->add(BOOST_TEST_CASE(test_void_promise));
    test->add(BOOST_TEST_CASE(test_reference_promise));
    test->add(BOOST_TEST_CASE(test_task_returning_void));
    test->add(BOOST_TEST_CASE(test_task_returning_reference));
    test->add(BOOST_TEST_CASE(test_shared_future));
    test->add(BOOST_TEST_CASE(test_copies_of_shared_future_become_ready_together));
    test->add(BOOST_TEST_CASE(test_shared_future_can_be_move_assigned_from_unique_future));
    test->add(BOOST_TEST_CASE(test_shared_future_void));
    test->add(BOOST_TEST_CASE(test_shared_future_ref));
    test->add(BOOST_TEST_CASE(test_can_get_a_second_future_from_a_moved_promise));
    test->add(BOOST_TEST_CASE(test_can_get_a_second_future_from_a_moved_void_promise));
//  test->add(BOOST_TEST_CASE(test_unique_future_for_move_only_udt));
    test->add(BOOST_TEST_CASE(test_unique_future_for_string));
    test->add(BOOST_TEST_CASE(test_wait_callback));
    test->add(BOOST_TEST_CASE(test_wait_callback_with_timed_wait));
    test->add(BOOST_TEST_CASE(test_wait_callback_for_packaged_task));
    test->add(BOOST_TEST_CASE(test_packaged_task_can_be_moved));
    test->add(BOOST_TEST_CASE(test_destroying_a_promise_stores_broken_promise));
    test->add(BOOST_TEST_CASE(test_destroying_a_packaged_task_stores_broken_promise));
    test->add(BOOST_TEST_CASE(test_wait_for_either_of_two_futures_1));
    test->add(BOOST_TEST_CASE(test_wait_for_either_of_two_futures_2));
    test->add(BOOST_TEST_CASE(test_wait_for_either_of_three_futures_1));
    test->add(BOOST_TEST_CASE(test_wait_for_either_of_three_futures_2));
    test->add(BOOST_TEST_CASE(test_wait_for_either_of_three_futures_3));
    test->add(BOOST_TEST_CASE(test_wait_for_either_of_four_futures_1));
    test->add(BOOST_TEST_CASE(test_wait_for_either_of_four_futures_2));
    test->add(BOOST_TEST_CASE(test_wait_for_either_of_four_futures_3));
    test->add(BOOST_TEST_CASE(test_wait_for_either_of_four_futures_4));
    test->add(BOOST_TEST_CASE(test_wait_for_either_of_five_futures_1));
    test->add(BOOST_TEST_CASE(test_wait_for_either_of_five_futures_2));
    test->add(BOOST_TEST_CASE(test_wait_for_either_of_five_futures_3));
    test->add(BOOST_TEST_CASE(test_wait_for_either_of_five_futures_4));
    test->add(BOOST_TEST_CASE(test_wait_for_either_of_five_futures_5));
    test->add(BOOST_TEST_CASE(test_wait_for_either_invokes_callbacks));
//  test->add(BOOST_TEST_CASE(test_wait_for_any_from_range));
//  test->add(BOOST_TEST_CASE(test_wait_for_all_from_range));
    test->add(BOOST_TEST_CASE(test_wait_for_all_two_futures));
    test->add(BOOST_TEST_CASE(test_wait_for_all_three_futures));
    test->add(BOOST_TEST_CASE(test_wait_for_all_four_futures));
    test->add(BOOST_TEST_CASE(test_wait_for_all_five_futures));
    test->add(BOOST_TEST_CASE(test_future_wait));

    return test;
}
