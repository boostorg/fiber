//  (C) Copyright 2008-9 Anthony Williams 
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TASKS_SPIN_FUTURE_HPP
#define BOOST_TASKS_SPIN_FUTURE_HPP

#include <algorithm>
#include <list>
#include <stdexcept>
#include <vector>

#include <boost/bind.hpp>
#include <boost/config.hpp>
#include <boost/exception_ptr.hpp>
#include <boost/function.hpp>
#include <boost/move/move.hpp>
#include <boost/next_prior.hpp>
#include <boost/ref.hpp>
#include <boost/scoped_array.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility/enable_if.hpp>

#include <boost/task/detail/future_traits.hpp>
#include <boost/task/spin/condition.hpp>
#include <boost/task/spin/mutex.hpp>

namespace boost {
namespace tasks {

namespace future_state {

    enum state { uninitialized, waiting, ready, moved };

}

namespace spin {
namespace detail {

struct future_object_base
{
    boost::exception_ptr exception;
    bool done;
    mutex mtx;
	spin::condition waiters;
    typedef std::list<spin::condition*> waiter_list;
    waiter_list external_waiters;
    boost::function<void()> callback;

    future_object_base():
        done(false)
    {}

    virtual ~future_object_base()
    {}

    waiter_list::iterator register_external_waiter(spin::condition& cv)
    {
        boost::unique_lock<mutex> lock(mtx);
        do_callback(lock);
        return external_waiters.insert(external_waiters.end(),&cv);
    }
    
    void remove_external_waiter(waiter_list::iterator it)
    {
        boost::lock_guard<mutex> lock(mtx);
        external_waiters.erase(it);
    }

    void mark_finished_internal()
    {
        done=true;
        waiters.notify_all();
        for(waiter_list::const_iterator it=external_waiters.begin(),
                end=external_waiters.end();it!=end;++it)
        {
            (*it)->notify_all();
        }
    }

    struct relocker
    {
        boost::unique_lock<mutex>& lock;
        
        relocker(boost::unique_lock<mutex>& lock_):
            lock(lock_)
        {
            lock.unlock();
        }
        ~relocker()
        {
            lock.lock();
        }
    };

    void do_callback(boost::unique_lock<mutex>& lock)
    {
        if(callback && !done)
        {
            boost::function<void()> local_callback=callback;
            relocker relock(lock);
            local_callback();
        }
    }
    

    void wait(bool rethrow=true)
    {
        boost::unique_lock<mutex> lock(mtx);
        do_callback(lock);
        while(!done)
        {
            waiters.wait(lock);
        }
        if(rethrow && exception)
        {
            boost::rethrow_exception(exception);
        }
    }

            bool timed_wait_until(boost::system_time const& target_time)
            {
                boost::unique_lock<mutex> lock(mtx);
                do_callback(lock);
                while(!done)
                {
                    bool const success=waiters.timed_wait(lock,target_time);
                    if(!success && !done)
                    {
                        return false;
                    }
                }
                return true;
            }
    
    void mark_exceptional_finish_internal(boost::exception_ptr const& e)
    {
        exception=e;
        mark_finished_internal();
    }
    void mark_exceptional_finish()
    {
        boost::lock_guard<mutex> lock(mtx);
        mark_exceptional_finish_internal(boost::current_exception());
    }

    bool has_value()
    {
        boost::lock_guard<mutex> lock(mtx);
        return done && !exception;
    }
    bool has_exception()
    {
        boost::lock_guard<mutex> lock(mtx);
        return done && exception;
    }

    template<typename F,typename U>
    void set_wait_callback(F f,U* u)
    {
        callback=boost::bind(f,boost::ref(*u));
    }
    
private:
    future_object_base(future_object_base const&);
    future_object_base& operator=(future_object_base const&);
};

template<typename T>
struct future_object: future_object_base
{
    typedef typename tasks::detail::future_traits<T>::storage_type storage_type;
    typedef typename tasks::detail::future_traits<T>::source_reference_type source_reference_type;
    typedef typename tasks::detail::future_traits<T>::rvalue_source_type rvalue_source_type;
    typedef typename tasks::detail::future_traits<T>::move_dest_type move_dest_type;
    
    storage_type result;

    future_object():
        result(0)
    {}

    void mark_finished_with_result_internal(source_reference_type result_)
		{
			tasks::detail::future_traits<T>::init(result,result_);
        mark_finished_internal();
    }
    void mark_finished_with_result_internal(rvalue_source_type result_)
    {
        tasks::detail::future_traits<T>::init(result,static_cast<rvalue_source_type>(result_));
        mark_finished_internal();
    }

    void mark_finished_with_result(source_reference_type result_)
    {
        boost::lock_guard<mutex> lock(mtx);
        mark_finished_with_result_internal(result_);
    }
    void mark_finished_with_result(rvalue_source_type result_)
    {
        boost::lock_guard<mutex> lock(mtx);
        mark_finished_with_result_internal(result_);
    }

    move_dest_type get()
    {
        wait();
        return *result;
    }

    future_state::state get_state()
    {
        boost::lock_guard<mutex> guard(mtx);
        if(!done)
        {
            return future_state::waiting;
        }
        else
        {
            return future_state::ready;
        }
    }

private:
    future_object(future_object const&);
    future_object& operator=(future_object const&);
};

template<>
struct future_object<void>: future_object_base
{
    future_object()
    {}

    void mark_finished_with_result_internal()
    {
        mark_finished_internal();
    }

    void mark_finished_with_result()
    {
        boost::lock_guard<mutex> lock(mtx);
        mark_finished_with_result_internal();
    }

    void get()
    {
        wait();
    }
    
    future_state::state get_state()
    {
        boost::lock_guard<mutex> guard(mtx);
        if(!done)
        {
            return future_state::waiting;
        }
        else
        {
            return future_state::ready;
        }
    }

private:
    future_object(future_object const&);
    future_object& operator=(future_object const&);
};

class future_waiter
{
    struct registered_waiter
    {
        boost::shared_ptr<detail::future_object_base> future;
        detail::future_object_base::waiter_list::iterator wait_iterator;
        unsigned index;

        registered_waiter(boost::shared_ptr<detail::future_object_base> const& future_,
                          detail::future_object_base::waiter_list::iterator wait_iterator_,
                          unsigned index_):
            future(future_),wait_iterator(wait_iterator_),index(index_)
        {}

    };
    
    struct all_futures_lock
    {
        unsigned count;
        boost::scoped_array<boost::unique_lock<mutex> > locks;
        
        all_futures_lock(std::vector<registered_waiter>& futures):
            count(futures.size()),locks(new boost::unique_lock<mutex>[count])
        {
            for(unsigned i=0;i<count;++i)
            {
                locks[i]=boost::unique_lock<mutex>(futures[i].future->mtx);
            }
        }
        
        void lock()
        {
            boost::lock(locks.get(),locks.get()+count);
        }
        
        void unlock()
        {
            for(unsigned i=0;i<count;++i)
            {
                locks[i].unlock();
            }
        }
    };
    
    condition cv;
    std::vector<registered_waiter> futures;
    unsigned future_count;
    
public:
    future_waiter():
        future_count(0)
    {}
    
    template<typename F>
    void add(F& f)
    {
        if(f.future)
        {
            futures.push_back(registered_waiter(f.future,f.future->register_external_waiter(cv),future_count));
        }
        ++future_count;
    }

    unsigned wait()
    {
        all_futures_lock lk(futures);
        for(;;)
        {
            for(unsigned i=0;i<futures.size();++i)
            {
                if(futures[i].future->done)
                {
                    return futures[i].index;
                }
            }
            cv.wait(lk);
        }
    }
    
    ~future_waiter()
    {
        for(unsigned i=0;i<futures.size();++i)
        {
            futures[i].future->remove_external_waiter(futures[i].wait_iterator);
        }
    }
    
};

}

template <typename R>
class unique_future;

template <typename R>
class shared_future;

template<typename T>
struct is_future_type
{
    BOOST_STATIC_CONSTANT(bool, value=false);
};

template<typename T>
struct is_future_type<unique_future<T> >
{
    BOOST_STATIC_CONSTANT(bool, value=true);
};

template<typename T>
struct is_future_type<shared_future<T> >
{
    BOOST_STATIC_CONSTANT(bool, value=true);
};

template<typename Iterator>
typename boost::disable_if<is_future_type<Iterator>,void>::type wait_for_all(Iterator begin,Iterator end)
{
    for(Iterator current=begin;current!=end;++current)
    {
        current->wait();
    }
}

template<typename F1,typename F2>
typename boost::enable_if<is_future_type<F1>,void>::type wait_for_all(F1& f1,F2& f2)
{
    f1.wait();
    f2.wait();
}

template<typename F1,typename F2,typename F3>
void wait_for_all(F1& f1,F2& f2,F3& f3)
{
    f1.wait();
    f2.wait();
    f3.wait();
}

template<typename F1,typename F2,typename F3,typename F4>
void wait_for_all(F1& f1,F2& f2,F3& f3,F4& f4)
{
    f1.wait();
    f2.wait();
    f3.wait();
    f4.wait();
}

template<typename F1,typename F2,typename F3,typename F4,typename F5>
void wait_for_all(F1& f1,F2& f2,F3& f3,F4& f4,F5& f5)
{
    f1.wait();
    f2.wait();
    f3.wait();
    f4.wait();
    f5.wait();
}

template<typename Iterator>
typename boost::disable_if<is_future_type<Iterator>,Iterator>::type wait_for_any(Iterator begin,Iterator end)
{
    detail::future_waiter waiter;
    for(Iterator current=begin;current!=end;++current)
    {
        waiter.add(*current);
    }
    return boost::next(begin,waiter.wait());
}

template<typename F1,typename F2>
typename boost::enable_if<is_future_type<F1>,unsigned>::type wait_for_any(F1& f1,F2& f2)
{
    detail::future_waiter waiter;
    waiter.add(f1);
    waiter.add(f2);
    return waiter.wait();
}

template<typename F1,typename F2,typename F3>
unsigned wait_for_any(F1& f1,F2& f2,F3& f3)
{
    detail::future_waiter waiter;
    waiter.add(f1);
    waiter.add(f2);
    waiter.add(f3);
    return waiter.wait();
}

template<typename F1,typename F2,typename F3,typename F4>
unsigned wait_for_any(F1& f1,F2& f2,F3& f3,F4& f4)
{
    detail::future_waiter waiter;
    waiter.add(f1);
    waiter.add(f2);
    waiter.add(f3);
    waiter.add(f4);
    return waiter.wait();
}

template<typename F1,typename F2,typename F3,typename F4,typename F5>
unsigned wait_for_any(F1& f1,F2& f2,F3& f3,F4& f4,F5& f5)
{
    detail::future_waiter waiter;
    waiter.add(f1);
    waiter.add(f2);
    waiter.add(f3);
    waiter.add(f4);
    waiter.add(f5);
    return waiter.wait();
}

template <typename R>
class promise;

template <typename R>
class packaged_task;

template <typename R>
class unique_future
{
    typedef boost::shared_ptr<detail::future_object<R> > future_ptr;

	BOOST_MOVABLE_BUT_NOT_COPYABLE( unique_future);
    
    future_ptr future;

    friend class shared_future<R>;
    friend class promise<R>;
    friend class packaged_task<R>;
    friend class detail::future_waiter;

    typedef typename tasks::detail::future_traits<R>::move_dest_type move_dest_type;

    unique_future(future_ptr future_):
        future(future_)
    {}

public:
    typedef future_state::state state;

    unique_future()
    {}
   
    ~unique_future()
    {}

	unique_future( BOOST_RV_REF( unique_future) other):
	    future(other.future)
	{
	    other.future.reset();
	}
	
	unique_future& operator=( BOOST_RV_REF( unique_future) other)
	{
	    future=other.future;
	    other.future.reset();
	    return *this;
	}
	
	void swap(unique_future& other)
	{
	    future.swap(other.future);
	}
	
	// retrieving the value
	move_dest_type get()
	{
	    if(!future)
	    {
	        throw future_uninitialized();
	    }
	
	    return future->get();
	}
	
	// functions to check state, and wait for ready
	state get_state() const
	{
	    if(!future)
	    {
	        return future_state::uninitialized;
	    }
	    return future->get_state();
	}
	
	
	bool is_ready() const
	{
	    return get_state()==future_state::ready;
	}
	
	bool has_exception() const
	{
	    return future && future->has_exception();
	}
	
	bool has_value() const
	{
	    return future && future->has_value();
	}
	
	void wait() const
	{
	    if(!future)
	    {
	        throw future_uninitialized();
	    }
	    future->wait(false);
	}
        
        template<typename Duration>
        bool timed_wait(Duration const& rel_time) const
        {
            return timed_wait_until(boost::get_system_time()+rel_time);
        }
        
        bool timed_wait_until(boost::system_time const& abs_time) const
        {
            if(!future)
            {
                throw future_uninitialized();
            }
            return future->timed_wait_until(abs_time);
        }
};

template <typename R>
class shared_future
{
    typedef boost::shared_ptr<detail::future_object<R> > future_ptr;

	BOOST_COPYABLE_AND_MOVABLE( shared_future);
    
    future_ptr future;

	friend class detail::future_waiter;
	friend class promise<R>;
	friend class packaged_task<R>;
	
	shared_future(future_ptr future_):
	    future(future_)
	{}
	
public:
	shared_future(shared_future const& other):
	    future(other.future)
	{}
	
	typedef future_state::state state;
	
	shared_future()
	{}
	
	~shared_future()
	{}
	
	shared_future& operator=( BOOST_COPY_ASSIGN_REF( shared_future) other)
	{
	    future=other.future;
	    return *this;
	}

	shared_future( BOOST_RV_REF( shared_future) other):
	    future(other.future)
	{
	    other->future.reset();
	}
	shared_future( BOOST_RV_REF( unique_future<R>) other):
	    future(other.future)
	{
	    other.future.reset();
	}
	shared_future& operator=(BOOST_RV_REF( shared_future) other)
	{
	    future.swap(other.future);
	    other.future.reset();
	    return *this;
	}
	shared_future& operator=( BOOST_RV_REF( unique_future<R>) other)
	{
	    future.swap(other.future);
	    other.future.reset();
	    return *this;
	}
	
	void swap(shared_future& other)
	{
	    future.swap(other.future);
	}
	
	// retrieving the value
	R get()
	{
	    if(!future)
	    {
	        throw future_uninitialized();
	    }
	
	    return future->get();
	}
	
	// functions to check state, and wait for ready
	state get_state() const
	{
	    if(!future)
	    {
	        return future_state::uninitialized;
	    }
	    return future->get_state();
	}
	
	
	bool is_ready() const
	{
	    return get_state()==future_state::ready;
	}
	
	bool has_exception() const
	{
	    return future && future->has_exception();
	}
	
	bool has_value() const
	{
	    return future && future->has_value();
	}
	
	void wait() const
	{
	    if(!future)
	    {
	        throw future_uninitialized();
	    }
	    future->wait(false);
	}
        
        template<typename Duration>
        bool timed_wait(Duration const& rel_time) const
        {
            return timed_wait_until(boost::get_system_time()+rel_time);
        }
        
        bool timed_wait_until(boost::system_time const& abs_time) const
        {
            if(!future)
            {
                throw future_uninitialized();
            }
            return future->timed_wait_until(abs_time);
        }
};

template <typename R>
class promise
{
    typedef boost::shared_ptr<detail::future_object<R> > future_ptr;

	BOOST_MOVABLE_BUT_NOT_COPYABLE( promise);
    
    future_ptr future;
    bool future_obtained;
    
    void lazy_init()
    {
        if(!future)
        {
            future_obtained=false;
            future.reset(new detail::future_object<R>);
        }
    }
    
public:
//         template <class Allocator> explicit promise(Allocator a);

	promise():
	    future(),future_obtained(false)
	{}
	
	~promise()
	{
	    if(future)
	    {
	        boost::lock_guard<mutex> lock(future->mtx);
	
	        if(!future->done)
	        {
	            future->mark_exceptional_finish_internal(boost::copy_exception(broken_promise()));
	        }
	    }
	}
	
	promise( BOOST_RV_REF( promise) rhs):
	    future(rhs.future),future_obtained(rhs.future_obtained)
	{
	    rhs.future.reset();
	}
	promise & operator=( BOOST_RV_REF( promise) rhs)
	{
	    future=rhs.future;
	    future_obtained=rhs.future_obtained;
	    rhs.future.reset();
	    return *this;
	}
        
	void swap(promise& other)
	{
	    future.swap(other.future);
	    std::swap(future_obtained,other.future_obtained);
	}
	
	// Result retrieval
	unique_future<R> get_future()
	{
	    lazy_init();
	    if(future_obtained)
	    {
	        throw future_already_retrieved();
	    }
	    future_obtained=true;
	    return unique_future<R>(future);
	}
	
	void set_value(typename tasks::detail::future_traits<R>::source_reference_type r)
	{
	    lazy_init();
	    boost::lock_guard<mutex> lock(future->mtx);
	    if(future->done)
	    {
	        throw promise_already_satisfied();
	    }
	    future->mark_finished_with_result_internal(r);
	}
	
//         void set_value(R && r);
	void set_value(typename tasks::detail::future_traits<R>::rvalue_source_type r)
	{
	    lazy_init();
	    boost::lock_guard<mutex> lock(future->mtx);
	    if(future->done)
	    {
	        throw promise_already_satisfied();
	    }
	    future->mark_finished_with_result_internal(static_cast<typename tasks::detail::future_traits<R>::rvalue_source_type>(r));
	}
	
	void set_exception(boost::exception_ptr p)
	{
	    lazy_init();
	    boost::lock_guard<mutex> lock(future->mtx);
	    if(future->done)
	    {
	        throw promise_already_satisfied();
	    }
	    future->mark_exceptional_finish_internal(p);
	}
	
	template<typename F>
	void set_wait_callback(F f)
	{
	    lazy_init();
	    future->set_wait_callback(f,this);
	}
	
};

template <>
class promise<void>
{
    typedef boost::shared_ptr<detail::future_object<void> > future_ptr;

	BOOST_MOVABLE_BUT_NOT_COPYABLE( promise);
    
    future_ptr future;
    bool future_obtained;
    
    void lazy_init()
    {
        if(!future)
        {
            future_obtained=false;
            future.reset(new detail::future_object<void>);
        }
    }
public:
	promise():
	    future(),future_obtained(false)
	{}
	
	~promise()
	{
	    if(future)
	    {
	        boost::lock_guard<mutex> lock(future->mtx);
	
	        if(!future->done)
	        {
	            future->mark_exceptional_finish_internal(boost::copy_exception(broken_promise()));
	        }
	    }
	}
	
	promise( BOOST_RV_REF( promise) rhs):
	    future(rhs.future),future_obtained(rhs.future_obtained)
	{
	    rhs.future.reset();
	}
	promise & operator=( BOOST_RV_REF( promise) rhs)
	{
	    future=rhs.future;
	    future_obtained=rhs.future_obtained;
	    rhs.future.reset();
	    return *this;
	}
	
	void swap(promise& other)
	{
	    future.swap(other.future);
	    std::swap(future_obtained,other.future_obtained);
	}
	
	// Result retrieval
	unique_future<void> get_future()
	{
	    lazy_init();
	    
	    if(future_obtained)
	    {
	        throw future_already_retrieved();
	    }
	    future_obtained=true;
	    return unique_future<void>(future);
	}
	
	void set_value()
	{
	    lazy_init();
	    boost::lock_guard<mutex> lock(future->mtx);
	    if(future->done)
	    {
	        throw promise_already_satisfied();
	    }
	    future->mark_finished_with_result_internal();
	}
	
	void set_exception(boost::exception_ptr p)
	{
	    lazy_init();
	    boost::lock_guard<mutex> lock(future->mtx);
	    if(future->done)
	    {
	        throw promise_already_satisfied();
	    }
	    future->mark_exceptional_finish_internal(p);
	}
	
	template<typename F>
	void set_wait_callback(F f)
	{
	    lazy_init();
	    future->set_wait_callback(f,this);
	}
};

namespace detail {

template<typename R>
struct task_base:
    detail::future_object<R>
{
    bool started;

    task_base():
        started(false)
    {}

    void run()
    {
        {
            boost::lock_guard<mutex> lk(this->mtx);
            if(started)
            {
                throw task_already_started();
            }
            started=true;
        }
        do_run();
    }

    void owner_destroyed()
    {
        boost::lock_guard<mutex> lk(this->mtx);
        if(!started)
        {
            started=true;
            this->mark_exceptional_finish_internal(boost::copy_exception(broken_promise()));
        }
    }
    
    virtual void do_run()=0;
};


template<typename R,typename F>
struct task_object:
    task_base<R>
{
    F f;
    task_object(F const& f_):
        f(f_)
    {}
    task_object( BOOST_RV_REF( F) f_):
        f(f_)
    {}
    
    void do_run()
    {
        try
        {
            this->mark_finished_with_result(f());
        }
        catch(...)
        {
            this->mark_exceptional_finish();
        }
    }
};

template<typename F>
struct task_object<void,F>:
    task_base<void>
{
    F f;
    task_object(F const& f_):
        f(f_)
    {}
    task_object( BOOST_RV_REF( F) f_):
        f(f_)
    {}
    
    void do_run()
    {
        try
        {
            f();
            this->mark_finished_with_result();
        }
        catch(...)
        {
            this->mark_exceptional_finish();
        }
    }
};

}


template<typename R>
class packaged_task
{
	BOOST_MOVABLE_BUT_NOT_COPYABLE( packaged_task);

    boost::shared_ptr<detail::task_base<R> > task;
    bool future_obtained;
    
public:
    packaged_task():
        future_obtained(false)
    {}
    
    // construction and destruction
    template <class F>
    explicit packaged_task(F const& f):
        task(new detail::task_object<R,F>(f)),future_obtained(false)
    {}
    explicit packaged_task(R(*f)()):
        task(new detail::task_object<R,R(*)()>(f)),future_obtained(false)
    {}
    
    template <class F>
    explicit packaged_task( BOOST_RV_REF( F) f):
        task(new detail::task_object<R,F>(f)),future_obtained(false)
    {}

//         template <class F, class Allocator>
//         explicit packaged_task(F const& f, Allocator a);
//         template <class F, class Allocator>
//         explicit packaged_task(F&& f, Allocator a);


	~packaged_task()
	{
	    if(task)
	    {
	        task->owner_destroyed();
	    }
	}

	packaged_task( BOOST_RV_REF( packaged_task) other):
	    future_obtained(other.future_obtained)
	{
	    task.swap(other.task);
	    other.future_obtained=false;
	}
	packaged_task& operator=( BOOST_RV_REF( packaged_task) other)
	{
	    packaged_task temp(other);
	    swap(temp);
	    return *this;
	}
	
	void swap(packaged_task& other)
	{
	    task.swap(other.task);
	    std::swap(future_obtained,other.future_obtained);
	}
	
	// result retrieval
	unique_future<R> get_future()
	{
	    if(!task)
	    {
	        throw task_moved();
	    }
	    else if(!future_obtained)
	    {
	        future_obtained=true;
	        return unique_future<R>(task);
	    }
	    else
	    {
	        throw future_already_retrieved();
	    }
	}
	
	
	// execution
	void operator()()
	{
	    if(!task)
	    {
	        throw task_moved();
	    }
	    task->run();
	}
	
	template<typename F>
	void set_wait_callback(F f)
	{
	    task->set_wait_callback(f,this);
	}
};

}}}

#endif // BOOST_TASKS_SPIN_FUTURE_H
